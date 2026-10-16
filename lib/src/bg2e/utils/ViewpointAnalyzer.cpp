/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <bg2e/utils/ViewpointAnalyzer.hpp>

#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/draw/Drawable.hpp>
#include <bg2e/draw/RenderQueue.hpp>
#include <bg2e/draw/RenderQueueVisitor.hpp>
#include <bg2e/gpu/CommandBuffer.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/GraphicsPipeline.hpp>
#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/PipelineLayout.hpp>
#include <bg2e/gpu/ShaderLib.hpp>
#include <bg2e/scene/BoundingBox.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Scene.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace bg2e::utils {

namespace {

std::vector<float> sampleValues(const ViewpointSampleRange& range)
{
    std::vector<float> values;
    values.reserve(range.samples);
    if (range.samples == 1)
    {
        values.push_back(range.min);
        return values;
    }
    const float divisor = range.cyclic ? static_cast<float>(range.samples) : static_cast<float>(range.samples - 1);
    const float step = (range.max - range.min) / divisor;
    for (uint32_t i = 0; i < range.samples; ++i)
    {
        values.push_back(range.min + static_cast<float>(i) * step);
    }
    return values;
}

uint32_t decodeIdentifier(const uint8_t* pixel)
{
    return uint32_t(pixel[0]) | (uint32_t(pixel[1]) << 8u) | (uint32_t(pixel[2]) << 16u);
}

float linearizeDepth(float depth, float nearPlane, float farPlane)
{
    const float denominator = farPlane - depth * (farPlane - nearPlane);
    return denominator > std::numeric_limits<float>::epsilon() ? nearPlane * farPlane / denominator : farPlane;
}

size_t checkedMultiply(size_t lhs, size_t rhs, const char* description)
{
    if (rhs != 0 && lhs > std::numeric_limits<size_t>::max() / rhs)
    {
        throw std::overflow_error(std::string("ViewpointAnalyzer: ") + description + " overflow");
    }
    return lhs * rhs;
}

struct PushConstants {
    glm::mat4 mvp{1.0f};
    glm::uvec4 identifier{0};
};

static_assert(sizeof(PushConstants) == 80);

}

struct ViewpointAnalyzer::DrawItem {
    std::shared_ptr<draw::Drawable::DrawMesh> mesh;
    glm::mat4 world{1.0f};
    uint32_t submesh = 0;
    uint32_t identifier = 0;
};

ViewpointAnalyzer::ViewpointAnalyzer(draw::Engine* engine)
    : _engine(engine)
{
    if (!_engine)
    {
        throw std::invalid_argument("ViewpointAnalyzer: engine is null");
    }
}

ViewpointAnalyzer::~ViewpointAnalyzer()
{
    cleanup();
}

void ViewpointAnalyzer::init()
{
    validateConfig();
    ensureResources();
}

void ViewpointAnalyzer::cleanup()
{
    _pipeline.reset();
    _pipelineLayout.reset();
    _identifierImage.reset();
    _depthImage.reset();
    _resourceWidth = 0;
    _resourceHeight = 0;
    _analyzedSubmeshes.clear();
}

void ViewpointAnalyzer::setConfig(const ViewpointSamplingConfig& config)
{
    const auto previous = _config;
    _config = config;
    try
    {
        validateConfig();
    }
    catch (...)
    {
        _config = previous;
        throw;
    }
}

void ViewpointAnalyzer::validateConfig() const
{
    if (_config.width == 0 || _config.height == 0)
    {
        throw std::invalid_argument("ViewpointAnalyzer: image dimensions must be greater than zero");
    }
    const auto validateRange = [](const ViewpointSampleRange& range, const char* name)
    {
        if (range.samples == 0 || !std::isfinite(range.min) || !std::isfinite(range.max) || range.max < range.min)
        {
            throw std::invalid_argument(std::string("ViewpointAnalyzer: invalid ") + name + " sample range");
        }
    };
    validateRange(_config.yaw, "yaw");
    validateRange(_config.pitch, "pitch");
    validateRange(_config.targetHeight, "target height");
    validateRange(_config.distanceScale, "distance scale");
    validateRange(_config.fieldOfView, "field of view");
    if (_config.pitch.min <= -89.9f || _config.pitch.max >= 89.9f)
    {
        throw std::invalid_argument("ViewpointAnalyzer: pitch must remain between -89.9 and 89.9 degrees");
    }
    if (_config.targetHeight.min < 0.0f || _config.targetHeight.max > 1.0f)
    {
        throw std::invalid_argument("ViewpointAnalyzer: target height must be normalized to [0, 1]");
    }
    if (_config.distanceScale.min <= 0.0f || _config.fieldOfView.min <= 0.0f || _config.fieldOfView.max >= 179.0f)
    {
        throw std::invalid_argument("ViewpointAnalyzer: distance scale and field of view must be positive and FOV below 179 degrees");
    }
}

std::vector<CameraCandidate> ViewpointAnalyzer::generateCandidates(const scene::BoundingBox& bounds) const
{
    validateConfig();
    if (!bounds.isValid())
    {
        throw std::invalid_argument("ViewpointAnalyzer::generateCandidates: invalid scene bounds");
    }
    const auto yawValues = sampleValues(_config.yaw);
    const auto pitchValues = sampleValues(_config.pitch);
    const auto targetValues = sampleValues(_config.targetHeight);
    const auto distanceValues = sampleValues(_config.distanceScale);
    const auto fovValues = sampleValues(_config.fieldOfView);
    size_t count = checkedMultiply(yawValues.size(), pitchValues.size(), "candidate count");
    count = checkedMultiply(count, targetValues.size(), "candidate count");
    count = checkedMultiply(count, distanceValues.size(), "candidate count");
    count = checkedMultiply(count, fovValues.size(), "candidate count");
    std::vector<CameraCandidate> result;
    result.reserve(count);
    const glm::vec3 size = bounds.size();
    const glm::vec3 center = bounds.center();
    for (float targetHeight : targetValues)
    {
        for (float fieldOfView : fovValues)
        {
            for (float distanceScale : distanceValues)
            {
                for (float pitch : pitchValues)
                {
                    for (float yaw : yawValues)
                    {
                        CameraCandidate candidate;
                        candidate.yaw = yaw;
                        candidate.pitch = pitch;
                        candidate.targetHeight = targetHeight;
                        candidate.distanceScale = distanceScale;
                        candidate.fieldOfView = fieldOfView;
                        candidate.target = {center.x, bounds.min().y + size.y * targetHeight, center.z};
                        const float verticalRadius = std::max(candidate.target.y - bounds.min().y, bounds.max().y - candidate.target.y);
                        const float radius = std::max(glm::length(glm::vec3(size.x * 0.5f, verticalRadius, size.z * 0.5f)), 0.001f);
                        candidate.distance = radius / std::sin(glm::radians(fieldOfView) * 0.5f) * distanceScale;
                        candidate.nearPlane = std::max(0.001f, candidate.distance - radius * 1.25f);
                        candidate.farPlane = candidate.distance + radius * 1.25f;
                        const float yawRadians = glm::radians(yaw);
                        const float pitchRadians = glm::radians(pitch);
                        const float horizontal = std::cos(pitchRadians);
                        const glm::vec3 direction{horizontal * std::sin(yawRadians), std::sin(pitchRadians), horizontal * std::cos(yawRadians)};
                        candidate.position = candidate.target + direction * candidate.distance;
                        candidate.viewMatrix = glm::lookAtLH(candidate.position, candidate.target, glm::vec3(0.0f, 1.0f, 0.0f));
                        candidate.projectionMatrix = glm::perspectiveLH_ZO(
                            glm::radians(fieldOfView), static_cast<float>(_config.width) / static_cast<float>(_config.height),
                            candidate.nearPlane, candidate.farPlane);
                        result.push_back(candidate);
                    }
                }
            }
        }
    }
    return result;
}

std::vector<ViewSample> ViewpointAnalyzer::analyze(scene::Node* rootNode)
{
    if (!rootNode)
    {
        throw std::invalid_argument("ViewpointAnalyzer::analyze: root node is null");
    }
    validateConfig();
    ensureResources();
    draw::RenderQueue queue;
    draw::RenderQueueVisitor extractor;
    extractor.enqueue(rootNode, *_engine, queue);
    _analyzedSubmeshes.clear();
    std::vector<DrawItem> drawItems;
    drawItems.reserve(queue.items().size());
    for (const auto& item : queue.items())
    {
        auto node = item.node.lock();
        if (!node || !item.drawMesh || item.submeshIndex >= item.drawMesh->submeshCount())
        {
            continue;
        }
        auto* component = node->drawable();
        auto drawable = component ? std::dynamic_pointer_cast<scene::DrawableComponent>(component->shared_from_this()) : nullptr;
        if (!drawable)
        {
            continue;
        }
        if (drawItems.size() >= 0x00ffffffu)
        {
            throw std::length_error("ViewpointAnalyzer: too many submeshes for RGBA8 identifiers");
        }
        const uint32_t id = static_cast<uint32_t>(drawItems.size() + 1);
        _analyzedSubmeshes.push_back({id, node, drawable, item.submeshIndex});
        drawItems.push_back({item.drawMesh, item.worldMatrix, item.submeshIndex, id});
    }
    if (drawItems.empty())
    {
        return {};
    }
    const scene::BoundingBox bounds(rootNode);
    const auto candidates = generateCandidates(bounds);
    const size_t pixelCount = checkedMultiply(_config.width, _config.height, "pixel count");
    const size_t rgbaBytes = checkedMultiply(pixelCount, 4, "identifier readback size");
    checkedMultiply(pixelCount, sizeof(float), "depth readback size");
    auto* device = _engine->device();
    if (!device || !device->isValid())
    {
        throw std::logic_error("ViewpointAnalyzer requires a live draw device");
    }
    device->waitIdle();
    std::vector<ViewSample> samples;
    samples.reserve(candidates.size());
    for (const auto& candidate : candidates)
    {
        device->immediateSubmit([&](gpu::CommandBuffer* command)
        {
            command->transition(_identifierImage.get(), gpu::ImageLayout::ColorAttachment);
            command->transition(_depthImage.get(), gpu::ImageLayout::DepthAttachment);
            gpu::RenderingDescription rendering;
            rendering.colors.push_back({.image = _identifierImage.get(), .load = gpu::AttachmentLoadOperation::Clear});
            rendering.depth = gpu::DepthAttachmentDescription{.image = _depthImage.get(), .load = gpu::AttachmentLoadOperation::Clear};
            command->beginRendering(rendering);
            command->clearColor(0, {0.0f, 0.0f, 0.0f, 0.0f});
            command->clearDepth(1.0f);
            command->bindPipeline(_pipeline.get());
            for (const auto& item : drawItems)
            {
                const PushConstants push{candidate.projectionMatrix * candidate.viewMatrix * item.world, {item.identifier, 0, 0, 0}};
                command->pushConstants(gpu::ShaderStage::Vertex, 0, sizeof(push), &push);
                command->bindVertexBuffer(0, item.mesh->vertexBuffer());
                command->bindIndexBuffer(item.mesh->indexBuffer());
                item.mesh->drawSubmesh(command, item.submesh);
            }
            command->endRendering();
            command->transition(_identifierImage.get(), gpu::ImageLayout::TransferSrc);
            command->transition(_depthImage.get(), gpu::ImageLayout::TransferSrc);
        });
        std::vector<uint8_t> identifierBytes;
        _identifierImage->readPixelsRGBA8(identifierBytes, gpu::ImageLayout::TransferSrc);
        std::vector<float> depths;
        _depthImage->readPixelsDepth32(depths, gpu::ImageLayout::TransferSrc);
        if (identifierBytes.size() < rgbaBytes || depths.size() < pixelCount)
        {
            throw std::runtime_error("ViewpointAnalyzer: incomplete ID or depth readback");
        }
        ViewSample sample;
        sample.camera = candidate;
        sample.analysis.pixelsPerSubmesh.resize(drawItems.size(), 0u);
        glm::uvec2 minPixel(_config.width, _config.height);
        glm::uvec2 maxPixel(0, 0);
        double mean = 0.0;
        double m2 = 0.0;
        float minDepth = std::numeric_limits<float>::max();
        float maxDepth = 0.0f;
        for (uint32_t y = 0; y < _config.height; ++y)
        {
            for (uint32_t x = 0; x < _config.width; ++x)
            {
                const size_t index = static_cast<size_t>(y) * _config.width + x;
                const uint32_t id = decodeIdentifier(identifierBytes.data() + index * 4);
                if (id == 0 || id > drawItems.size())
                {
                    continue;
                }
                ++sample.analysis.visiblePixelCount;
                ++sample.analysis.pixelsPerSubmesh[id - 1];
                if (x == 0 || y == 0 || x + 1 == _config.width || y + 1 == _config.height)
                {
                    ++sample.analysis.borderPixelCount;
                }
                minPixel = glm::min(minPixel, glm::uvec2(x, y));
                maxPixel = glm::max(maxPixel, glm::uvec2(x, y));
                const float linearDepth = linearizeDepth(depths[index], candidate.nearPlane, candidate.farPlane);
                minDepth = std::min(minDepth, linearDepth);
                maxDepth = std::max(maxDepth, linearDepth);
                const double delta = linearDepth - mean;
                mean += delta / sample.analysis.visiblePixelCount;
                m2 += delta * (linearDepth - mean);
            }
        }
        for (uint32_t count : sample.analysis.pixelsPerSubmesh)
        {
            sample.analysis.visibleSubmeshCount += count > 0 ? 1u : 0u;
        }
        if (sample.analysis.visiblePixelCount > 0)
        {
            sample.analysis.minPixel = minPixel;
            sample.analysis.maxPixel = maxPixel;
            sample.analysis.minDepth = minDepth;
            sample.analysis.maxDepth = maxDepth;
            sample.analysis.meanDepth = static_cast<float>(mean);
            sample.analysis.depthVariance = static_cast<float>(m2 / sample.analysis.visiblePixelCount);
        }
        sample.score = score(sample);
        samples.push_back(std::move(sample));
    }
    return samples;
}

std::vector<ViewSample> ViewpointAnalyzer::analyze(scene::Scene* scene)
{
    if (!scene)
    {
        throw std::invalid_argument("ViewpointAnalyzer::analyze: scene is null");
    }
    return analyze(scene->rootNode());
}

const ViewSample* ViewpointAnalyzer::bestSample(const std::vector<ViewSample>& samples)
{
    if (samples.empty())
    {
        return nullptr;
    }
    return &*std::max_element(samples.begin(), samples.end(), [](const ViewSample& lhs, const ViewSample& rhs)
    {
        return lhs.score < rhs.score;
    });
}

float ViewpointAnalyzer::score(const ViewSample& sample) const
{
    const float totalPixels = static_cast<float>(_config.width) * static_cast<float>(_config.height);
    const float coverage = static_cast<float>(sample.analysis.visiblePixelCount) / totalPixels;
    const float border = static_cast<float>(sample.analysis.borderPixelCount) / totalPixels;
    return coverage - border * 2.0f;
}

void ViewpointAnalyzer::ensureResources()
{
    if (!_engine->device() || !_engine->device()->isValid())
    {
        throw std::logic_error("ViewpointAnalyzer::init requires an initialized draw engine");
    }
    if (_identifierImage && _depthImage && _pipelineLayout && _pipeline &&
        _resourceWidth == _config.width && _resourceHeight == _config.height)
    {
        return;
    }
    createResources();
}

void ViewpointAnalyzer::createResources()
{
    cleanup();
    auto* device = _engine->device();
    gpu::ImageDescription image;
    image.size = {_config.width, _config.height};
    image.format = gpu::PixelFormat::R8G8B8A8_UNORM;
    image.usage = gpu::ImageUsage::ColorAttachment | gpu::ImageUsage::TransferSrc;
    _identifierImage = device->createImage(image);
    image.format = gpu::PixelFormat::D32_SFLOAT;
    image.usage = gpu::ImageUsage::DepthStencil | gpu::ImageUsage::TransferSrc;
    _depthImage = device->createImage(image);
    gpu::PipelineLayoutDescription layoutDescription;
    layoutDescription.pushConstants.push_back({0, sizeof(PushConstants), gpu::ShaderStage::Vertex});
    _pipelineLayout = device->createPipelineLayout(layoutDescription);
    gpu::ShaderLib shaders(base::PlatformTools::shaderPath(), _engine->backendType());
    auto vertex = shaders.vertex("draw/picking/picking", device, "ViewpointAnalyzer vertex");
    auto fragment = shaders.fragment("draw/picking/picking", device, "ViewpointAnalyzer fragment");
    gpu::GraphicsPipelineDescription description;
    description.vertexShader = vertex.get();
    description.fragmentShader = fragment.get();
    description.layout = _pipelineLayout.get();
    description.colorFormat = gpu::PixelFormat::R8G8B8A8_UNORM;
    description.depthFormat = gpu::PixelFormat::D32_SFLOAT;
    description.cullMode = gpu::CullMode::None;
    description.depthState = gpu::DepthState{true, true, gpu::CompareOperation::LessOrEqual};
    description.addVertexBufferDescription(draw::Drawable::DrawMesh::vertexBufferDescription());
    _pipeline = device->createGraphicsPipeline(description);
    _resourceWidth = _config.width;
    _resourceHeight = _config.height;
}

}
