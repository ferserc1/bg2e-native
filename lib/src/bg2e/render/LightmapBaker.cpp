#include <bg2e/render/LightmapBaker.hpp>

#include <bg2e/render/BakerContext.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/scene/Drawable.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/Mesh.hpp>
#include <bg2e/scene/Node.hpp>
#include "UvSurfacePass.hpp"
#include "UvTemporalAccumulator.hpp"
#include "UvAtlasValidation.hpp"

#include <stdexcept>
#include <string>
#include <utility>
#include <limits>
#include <cmath>

namespace bg2e {
namespace render {

LightmapBaker::LightmapBaker(std::shared_ptr<BakerContext> context,
                             std::shared_ptr<scene::Node> targetNode,
                             LightmapSettings settings)
    : _context(std::move(context)),
      _targetNode(std::move(targetNode)),
      _settings(settings)
{
    validateTarget();
    _uvSurfacePass = std::make_shared<UvSurfacePass>(
        _context->engine(), VkExtent2D{ _settings.resolution, _settings.resolution });
    _accumulator = std::make_shared<UvTemporalAccumulator>(
        _context->engine(), VkExtent2D{ _settings.resolution, _settings.resolution });
    const uint32_t frameSlots = _context->engine()->numImages();
    _aoImages.resize(frameSlots);
    _giImages.resize(frameSlots);
    _resultImages.resize(frameSlots);
    for (uint32_t frameSlot = 0; frameSlot < frameSlots; ++frameSlot)
    {
        _aoImages[frameSlot] = std::shared_ptr<vulkan::Image>(
            vulkan::Image::createAllocatedImage(
                _context->engine(),
                "LightmapBaker RTAO result " + std::to_string(frameSlot),
                VK_FORMAT_R8_UNORM,
                VkExtent2D{ _settings.resolution, _settings.resolution },
                VK_IMAGE_USAGE_STORAGE_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT |
                VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT
            )
        );
        _giImages[frameSlot] = std::shared_ptr<vulkan::Image>(
            vulkan::Image::createAllocatedImage(
                _context->engine(),
                "LightmapBaker RTGI result " + std::to_string(frameSlot),
                VK_FORMAT_R16G16B16A16_SFLOAT,
                VkExtent2D{ _settings.resolution, _settings.resolution },
                VK_IMAGE_USAGE_STORAGE_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT |
                VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT
            )
        );
        _resultImages[frameSlot] = std::shared_ptr<vulkan::Image>(
            vulkan::Image::createAllocatedImage(
                _context->engine(),
                "LightmapBaker composed RGB multiplier " + std::to_string(frameSlot),
                VK_FORMAT_R16G16B16A16_SFLOAT,
                VkExtent2D{ _settings.resolution, _settings.resolution },
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                VK_IMAGE_USAGE_STORAGE_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT |
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT
            )
        );
    }
}

LightmapBaker::~LightmapBaker()
{
    if (!_uvSurfacePass || !_context || !_context->engine())
    {
        return;
    }
    auto pass = std::move(_uvSurfacePass);
    auto accumulator = std::move(_accumulator);
    auto aoImages = std::move(_aoImages);
    auto giImages = std::move(_giImages);
    auto resultImages = std::move(_resultImages);
    _context->engine()->deferredExec([
        pass = std::move(pass),
        accumulator = std::move(accumulator),
        aoImages = std::move(aoImages),
        giImages = std::move(giImages),
        resultImages = std::move(resultImages)]() mutable {
        pass.reset();
        accumulator.reset();
        aoImages.clear();
        giImages.clear();
        resultImages.clear();
    });
}

void LightmapBaker::validateTarget() const
{
    if (!_context) {
        throw std::invalid_argument("LightmapBaker: context must not be null");
    }
    if (!_targetNode) {
        throw std::invalid_argument("LightmapBaker: target node must not be null");
    }
    if (_targetNode->sceneRoot() != _context->rootNode()) {
        throw std::invalid_argument("LightmapBaker: target node belongs to a different scene root");
    }

    const auto component = _targetNode->drawable();
    const auto drawable = component ? component->drawable() : std::shared_ptr<scene::Drawable>{};
    if (!drawable) {
        throw std::invalid_argument("LightmapBaker: target node must contain a standard scene::Drawable");
    }
    const auto mesh = drawable->mesh();
    if (!mesh || drawable->submeshesCount() == 0 || mesh->vertices.empty() || mesh->indices.empty()) {
        throw std::invalid_argument("LightmapBaker: target Drawable must contain nonempty triangle submeshes");
    }
    if (mesh->submeshes.size() != drawable->submeshesCount()) {
        throw std::invalid_argument("LightmapBaker: target Drawable submesh metadata does not match its CPU mesh");
    }
    if (!drawable->isLoaded()) {
        throw std::invalid_argument("LightmapBaker: target Drawable must be loaded on the context engine");
    }

    for (uint32_t submeshIndex = 0; submeshIndex < drawable->submeshesCount(); ++submeshIndex) {
        const auto submesh = drawable->submeshData(submeshIndex);
        const uint64_t end = static_cast<uint64_t>(submesh.firstIndex) + submesh.indexCount;
        if (submesh.indexCount == 0 || submesh.indexCount % 3 != 0 || end > mesh->indices.size()) {
            throw std::invalid_argument("LightmapBaker: target Drawable has an invalid triangle submesh index range");
        }
        for (uint64_t index = submesh.firstIndex; index < end; ++index) {
            if (mesh->indices[static_cast<size_t>(index)] >= mesh->vertices.size()) {
                throw std::invalid_argument("LightmapBaker: target Drawable has an index outside its vertex array");
            }
        }
    }

    if (_settings.resolution == 0) {
        throw std::invalid_argument("LightmapBaker: resolution must be positive");
    }
    if (_settings.accumulationFrames == 0) {
        throw std::invalid_argument("LightmapBaker: accumulationFrames must be positive");
    }
    if (_settings.samplesPerPixel == 0) {
        throw std::invalid_argument("LightmapBaker: samplesPerPixel must be positive");
    }
    if (_settings.samplesPerPixel > static_cast<uint32_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("LightmapBaker: samplesPerPixel exceeds the RTAO shader limit");
    }
    if (_settings.mode == LightmapMode::RTGI &&
        (!std::isfinite(_settings.maxRayDistance) || _settings.maxRayDistance <= 0.0f)) {
        throw std::invalid_argument("LightmapBaker: maxRayDistance must be finite and positive");
    }
    if (_settings.mode != LightmapMode::RTAO && _settings.mode != LightmapMode::RTGI) {
        throw std::invalid_argument("LightmapBaker: mode has an unsupported value");
    }
    if (_settings.cpuFormat != LightmapPixelFormat::RGB8 &&
        _settings.cpuFormat != LightmapPixelFormat::RGB32F) {
        throw std::invalid_argument("LightmapBaker: cpuFormat has an unsupported value");
    }

    std::string uvError;
    if (!detail::validateUsableUv2(*drawable, uvError))
    {
        throw std::invalid_argument("LightmapBaker: " + uvError);
    }
}

void LightmapBaker::recordUvSurface(VkCommandBuffer cmd, uint32_t frameSlot)
{
    if (!_uvSurfacePass || !_targetNode)
    {
        throw std::logic_error("LightmapBaker: UV surface resources are unavailable");
    }
    if (_targetNode->sceneRoot() != _context->rootNode())
    {
        throw std::invalid_argument("LightmapBaker: target node no longer belongs to the context root");
    }
    const auto component = _targetNode->drawable();
    const auto drawable = component ? component->drawable() : std::shared_ptr<scene::Drawable>{};
    if (!drawable)
    {
        throw std::invalid_argument("LightmapBaker: target no longer contains a standard Drawable");
    }
    _uvSurfacePass->record(cmd, *drawable, _targetNode->worldMatrix(), frameSlot);
}

vulkan::Image& LightmapBaker::aoImage(uint32_t frameSlot)
{
    if (frameSlot >= _aoImages.size() || !_aoImages[frameSlot])
    {
        throw std::out_of_range("LightmapBaker: AO image frame slot is out of range");
    }
    return *_aoImages[frameSlot];
}

vulkan::Image& LightmapBaker::giImage(uint32_t frameSlot)
{
    if (frameSlot >= _giImages.size() || !_giImages[frameSlot])
    {
        throw std::out_of_range("LightmapBaker: GI image frame slot is out of range");
    }
    return *_giImages[frameSlot];
}

vulkan::Image& LightmapBaker::resultImage(uint32_t frameSlot)
{
    if (frameSlot >= _resultImages.size() || !_resultImages[frameSlot])
    {
        throw std::out_of_range("LightmapBaker: result image frame slot is out of range");
    }
    return *_resultImages[frameSlot];
}

void LightmapBaker::markResultImage(uint32_t frameSlot)
{
    if (frameSlot >= _resultImages.size())
    {
        throw std::out_of_range("LightmapBaker: result image frame slot is out of range");
    }
    _resultFrameNumber = _context->engine()->currentFrame();
    _hasResultImage = true;
    ++_completedFrames;
}

void LightmapBaker::recordAccumulation(VkCommandBuffer cmd,
                                       vulkan::DescriptorSetAllocator& descriptors,
                                       uint32_t frameSlot)
{
    _accumulator->record(cmd, descriptors, resultImage(frameSlot),
                         _uvSurfacePass->manager(frameSlot), _completedFrames);
}

void LightmapBaker::resetAccumulation()
{
    _completedFrames = 0;
    _hasResultImage = false;
    _accumulator->reset();
}

LightmapPixels LightmapBaker::readPixels() const
{
    if (!_hasResultImage)
    {
        throw std::logic_error("LightmapBaker: no bake result is available");
    }
    if (_context->engine()->currentFrame() == _resultFrameNumber)
    {
        throw std::logic_error("LightmapBaker: submit the last update before reading pixels");
    }
    // The result may have been written by any in-flight slot. Waiting for the
    // device is conservative, but avoids assuming a particular frame fence or
    // queuing a transfer ahead of the application's pending submission.
    _context->engine()->device().waitIdle();
    return _accumulator->readPixels(_settings.cpuFormat);
}

std::shared_ptr<vulkan::Image> LightmapBaker::image() const
{
    if (!_hasResultImage)
    {
        throw std::logic_error("LightmapBaker: no bake result image is available yet");
    }
    return _accumulator->image();
}

}
}
