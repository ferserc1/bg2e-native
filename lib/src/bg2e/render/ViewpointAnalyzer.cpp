/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 */

#include <bg2e/render/ViewpointAnalyzer.hpp>

#include <bg2e/render/Engine.hpp>
#include <bg2e/render/vulkan/all.hpp>
#include <bg2e/scene/BoundingBox.hpp>
#include <bg2e/scene/Drawable.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/Scene.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace bg2e::render {

namespace {

constexpr VkFormat IdentifierFormat = VK_FORMAT_R8G8B8A8_UNORM;
constexpr VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT;

std::vector<float> sampleValues(const ViewpointSampleRange& range)
{
    std::vector<float> result;
    result.reserve(range.samples);

    if (range.samples == 1)
    {
        result.push_back(range.min);
        return result;
    }

    const float denominator = range.cyclic
        ? static_cast<float>(range.samples)
        : static_cast<float>(range.samples - 1);
    const float step = (range.max - range.min) / denominator;
    for (uint32_t i = 0; i < range.samples; ++i)
    {
        result.push_back(range.min + static_cast<float>(i) * step);
    }
    return result;
}

uint32_t decodeIdentifier(const uint8_t * pixel)
{
    return static_cast<uint32_t>(pixel[0]) |
        (static_cast<uint32_t>(pixel[1]) << 8u) |
        (static_cast<uint32_t>(pixel[2]) << 16u) |
        (static_cast<uint32_t>(pixel[3]) << 24u);
}

float linearizeDepth(float depth, float nearPlane, float farPlane)
{
    const float denominator = farPlane - depth * (farPlane - nearPlane);
    return denominator > std::numeric_limits<float>::epsilon()
        ? nearPlane * farPlane / denominator
        : farPlane;
}

size_t checkedMultiply(size_t lhs, size_t rhs, const char * description)
{
    if (rhs != 0 && lhs > std::numeric_limits<size_t>::max() / rhs)
    {
        throw std::overflow_error(std::string("ViewpointAnalyzer: ") + description + " overflow");
    }
    return lhs * rhs;
}

}

struct ViewpointAnalyzer::DrawItem
{
    std::shared_ptr<scene::Drawable> drawable;
    glm::mat4 modelMatrix { 1.0f };
    uint32_t submesh = 0;
    uint32_t identifier = 0;
};

ViewpointAnalyzer::ViewpointAnalyzer(Engine * engine)
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
    cleanupImages();
    _analyzedSubmeshes.clear();

    if (!_engine)
    {
        return;
    }

    const auto device = _engine->device().handle();
    if (_pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device, _pipeline, nullptr);
        _pipeline = VK_NULL_HANDLE;
    }
    if (_pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(device, _pipelineLayout, nullptr);
        _pipelineLayout = VK_NULL_HANDLE;
    }
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

    const auto validateRange = [](const ViewpointSampleRange& range, const char * name)
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

    size_t candidateCount = checkedMultiply(yawValues.size(), pitchValues.size(), "candidate count");
    candidateCount = checkedMultiply(candidateCount, targetValues.size(), "candidate count");
    candidateCount = checkedMultiply(candidateCount, distanceValues.size(), "candidate count");
    candidateCount = checkedMultiply(candidateCount, fovValues.size(), "candidate count");
    std::vector<CameraCandidate> result;
    result.reserve(candidateCount);

    const glm::vec3 size = bounds.size();
    const glm::vec3 center = bounds.center();

    for (float targetHeight : targetValues)
    for (float fieldOfView : fovValues)
    for (float distanceScale : distanceValues)
    for (float pitch : pitchValues)
    for (float yaw : yawValues)
    {
        CameraCandidate candidate;
        candidate.yaw = yaw;
        candidate.pitch = pitch;
        candidate.targetHeight = targetHeight;
        candidate.distanceScale = distanceScale;
        candidate.fieldOfView = fieldOfView;
        candidate.target = { center.x, bounds.min().y + size.y * targetHeight, center.z };

        // Moving the target vertically moves it away from the AABB center. Fit
        // the sphere around the actual target so target-height optimization does
        // not accidentally change framing or clip the object.
        const float verticalRadius = std::max(
            candidate.target.y - bounds.min().y,
            bounds.max().y - candidate.target.y
        );
        const float radius = std::max(glm::length(glm::vec3(
            size.x * 0.5f, verticalRadius, size.z * 0.5f
        )), 0.001f);
        const float halfFov = glm::radians(fieldOfView) * 0.5f;
        candidate.distance = radius / std::sin(halfFov) * distanceScale;
        candidate.nearPlane = std::max(0.001f, candidate.distance - radius * 1.25f);
        candidate.farPlane = candidate.distance + radius * 1.25f;

        const float yawRadians = glm::radians(yaw);
        const float pitchRadians = glm::radians(pitch);
        const float horizontal = std::cos(pitchRadians);
        const glm::vec3 direction {
            horizontal * std::sin(yawRadians),
            std::sin(pitchRadians),
            horizontal * std::cos(yawRadians)
        };

        candidate.position = candidate.target + direction * candidate.distance;
        candidate.viewMatrix = glm::lookAtLH(candidate.position, candidate.target, glm::vec3(0.0f, 1.0f, 0.0f));
        candidate.projectionMatrix = glm::perspectiveLH_ZO(
            glm::radians(fieldOfView),
            static_cast<float>(_config.width) / static_cast<float>(_config.height),
            candidate.nearPlane,
            candidate.farPlane
        );
        result.push_back(candidate);
    }

    return result;
}

void ViewpointAnalyzer::collectDrawItems(scene::Node * node, std::vector<DrawItem>& items)
{
    if (!node || node->disabled())
    {
        return;
    }

    auto drawableComponent = node->drawable();
    auto drawable = drawableComponent ? drawableComponent->drawable() : nullptr;
    if (drawable && drawable->isLoaded())
    {
        auto drawableShared = std::dynamic_pointer_cast<scene::DrawableComponent>(drawableComponent->shared_from_this());
        for (uint32_t submesh = 0; submesh < drawable->submeshesCount(); ++submesh)
        {
            if (!drawable->submeshVisibility(submesh))
            {
                continue;
            }
            if (_analyzedSubmeshes.size() >= std::numeric_limits<uint32_t>::max() - 1u)
            {
                throw std::overflow_error("ViewpointAnalyzer: too many submeshes for 32-bit identifiers");
            }

            const uint32_t identifier = static_cast<uint32_t>(_analyzedSubmeshes.size() + 1u);
            _analyzedSubmeshes.push_back({
                .identifier = identifier,
                .node = node->weak_from_this(),
                .drawable = drawableShared,
                .submesh = submesh
            });
            items.push_back({
                .drawable = drawable,
                .modelMatrix = node->worldMatrix() * drawable->localSubmeshTransform(submesh),
                .submesh = submesh,
                .identifier = identifier
            });
        }
    }

    for (const auto& child : node->children())
    {
        collectDrawItems(child.get(), items);
    }
}

std::vector<ViewSample> ViewpointAnalyzer::analyze(scene::Node * rootNode)
{
    if (!rootNode)
    {
        throw std::invalid_argument("ViewpointAnalyzer::analyze: root node is null");
    }

    validateConfig();
    ensureResources();

    std::vector<DrawItem> drawItems;
    _analyzedSubmeshes.clear();
    collectDrawItems(rootNode, drawItems);
    if (drawItems.empty())
    {
        return {};
    }

    const scene::BoundingBox bounds(rootNode);
    const auto candidates = generateCandidates(bounds);

    const size_t pixelCount = checkedMultiply(_config.width, _config.height, "pixel count");
    const size_t bytesPerView = checkedMultiply(pixelCount, sizeof(uint32_t), "view readback size");
    const size_t batchBytes = checkedMultiply(bytesPerView, candidates.size(), "batch readback size");

    auto identifierReadback = std::unique_ptr<vulkan::Buffer>(vulkan::Buffer::createAllocatedBuffer(
        _engine, batchBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU,
        "ViewpointAnalyzer identifier readback"
    ));
    auto depthReadback = std::unique_ptr<vulkan::Buffer>(vulkan::Buffer::createAllocatedBuffer(
        _engine, batchBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU,
        "ViewpointAnalyzer depth readback"
    ));

    _engine->command().immediateSubmit([&](VkCommandBuffer cmd)
    {
        VkImageLayout identifierLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageLayout depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        for (size_t viewIndex = 0; viewIndex < candidates.size(); ++viewIndex)
        {
            vulkan::Image::cmdTransitionImage(
                cmd, _identifierImage->handle(), identifierLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                vulkan::Image::TransitionInfo(VK_IMAGE_ASPECT_COLOR_BIT)
            );
            vulkan::Image::cmdTransitionImage(
                cmd, _depthImage->handle(), depthLayout, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                vulkan::Image::TransitionInfo(VK_IMAGE_ASPECT_DEPTH_BIT)
            );

            VkClearValue identifierClear {};
            auto colorAttachment = vulkan::Info::attachmentInfo(_identifierImage->imageView(), &identifierClear);
            auto depthAttachment = vulkan::Info::depthAttachmentInfo(_depthImage->imageView(), 1.0f);
            auto renderingInfo = vulkan::Info::renderingInfo(_extent, &colorAttachment, &depthAttachment);
            vulkan::cmdBeginRendering(cmd, &renderingInfo);
            vulkan::macros::cmdSetDefaultViewportAndScissor(cmd, _extent);
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

            const auto& camera = candidates[viewIndex];
            for (const auto& item : drawItems)
            {
                PushConstantData pushConstants;
                pushConstants.mvp = camera.projectionMatrix * camera.viewMatrix * item.modelMatrix;
                pushConstants.identifier = item.identifier;
                vkCmdPushConstants(
                    cmd, _pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                    0, sizeof(PushConstantData), &pushConstants
                );
                item.drawable->renderMesh()->drawSubmesh(cmd, item.submesh);
            }
            vulkan::cmdEndRendering(cmd);

            vulkan::Image::cmdTransitionImage(
                cmd, _identifierImage->handle(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                vulkan::Image::TransitionInfo(VK_IMAGE_ASPECT_COLOR_BIT)
            );
            vulkan::Image::cmdTransitionImage(
                cmd, _depthImage->handle(), VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                vulkan::Image::TransitionInfo(VK_IMAGE_ASPECT_DEPTH_BIT)
            );

            VkBufferImageCopy identifierCopy {};
            identifierCopy.bufferOffset = viewIndex * bytesPerView;
            identifierCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            identifierCopy.imageSubresource.layerCount = 1;
            identifierCopy.imageExtent = { _config.width, _config.height, 1 };
            vkCmdCopyImageToBuffer(
                cmd, _identifierImage->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                identifierReadback->handle(), 1, &identifierCopy
            );

            VkBufferImageCopy depthCopy = identifierCopy;
            depthCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            vkCmdCopyImageToBuffer(
                cmd, _depthImage->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                depthReadback->handle(), 1, &depthCopy
            );

            identifierLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            depthLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        }
    });

    VK_ASSERT(vmaInvalidateAllocation(_engine->allocator(), identifierReadback->allocation(), 0, VK_WHOLE_SIZE));
    VK_ASSERT(vmaInvalidateAllocation(_engine->allocator(), depthReadback->allocation(), 0, VK_WHOLE_SIZE));
    const auto * identifierData = static_cast<const uint8_t *>(identifierReadback->allocatedData());
    const auto * depthData = static_cast<const float *>(depthReadback->allocatedData());

    std::vector<ViewSample> samples;
    samples.reserve(candidates.size());
    for (size_t viewIndex = 0; viewIndex < candidates.size(); ++viewIndex)
    {
        ViewSample sample;
        sample.camera = candidates[viewIndex];
        sample.analysis.pixelsPerSubmesh.resize(drawItems.size(), 0u);

        glm::uvec2 minPixel(_config.width, _config.height);
        glm::uvec2 maxPixel(0, 0);
        double depthMean = 0.0;
        double depthM2 = 0.0;
        float minDepth = std::numeric_limits<float>::max();
        float maxDepth = 0.0f;

        const auto * viewIdentifiers = identifierData + viewIndex * bytesPerView;
        const auto * viewDepth = depthData + viewIndex * pixelCount;
        for (uint32_t y = 0; y < _config.height; ++y)
        for (uint32_t x = 0; x < _config.width; ++x)
        {
            const size_t pixelIndex = static_cast<size_t>(y) * _config.width + x;
            const uint32_t identifier = decodeIdentifier(viewIdentifiers + pixelIndex * 4u);
            if (identifier == 0 || identifier > drawItems.size())
            {
                continue;
            }

            ++sample.analysis.visiblePixelCount;
            ++sample.analysis.pixelsPerSubmesh[identifier - 1u];
            if (x == 0 || y == 0 || x + 1 == _config.width || y + 1 == _config.height)
            {
                ++sample.analysis.borderPixelCount;
            }
            minPixel = glm::min(minPixel, glm::uvec2(x, y));
            maxPixel = glm::max(maxPixel, glm::uvec2(x, y));

            const float linearDepth = linearizeDepth(
                viewDepth[pixelIndex], sample.camera.nearPlane, sample.camera.farPlane
            );
            minDepth = std::min(minDepth, linearDepth);
            maxDepth = std::max(maxDepth, linearDepth);
            const double delta = linearDepth - depthMean;
            depthMean += delta / sample.analysis.visiblePixelCount;
            depthM2 += delta * (linearDepth - depthMean);
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
            sample.analysis.meanDepth = static_cast<float>(depthMean);
            sample.analysis.depthVariance = static_cast<float>(depthM2 / sample.analysis.visiblePixelCount);
        }
        sample.score = score(sample);
        samples.push_back(std::move(sample));
    }

    return samples;
}

std::vector<ViewSample> ViewpointAnalyzer::analyze(scene::Scene * scene)
{
    if (!scene)
    {
        throw std::invalid_argument("ViewpointAnalyzer::analyze: scene is null");
    }
    return analyze(scene->rootNode());
}

float ViewpointAnalyzer::score(const ViewSample& sample) const
{
    const float pixelCount = static_cast<float>(_config.width) * static_cast<float>(_config.height);
    const float coverage = static_cast<float>(sample.analysis.visiblePixelCount) / pixelCount;
    const float clipped = static_cast<float>(sample.analysis.borderPixelCount) / pixelCount;
    return coverage - clipped * 2.0f;
}

const ViewSample * ViewpointAnalyzer::bestSample(const std::vector<ViewSample>& samples)
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

void ViewpointAnalyzer::ensureResources()
{
    if (_extent.width != _config.width || _extent.height != _config.height)
    {
        cleanupImages();
        createImages();
    }
    if (_pipeline == VK_NULL_HANDLE)
    {
        createPipeline();
    }
}

void ViewpointAnalyzer::createImages()
{
    _extent = { _config.width, _config.height };
    _identifierImage = std::shared_ptr<vulkan::Image>(vulkan::Image::createAllocatedImage(
        _engine, "ViewpointAnalyzer identifier image", IdentifierFormat, _extent,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT, 1, false, 1, VK_SAMPLE_COUNT_1_BIT
    ));
    _depthImage = std::shared_ptr<vulkan::Image>(vulkan::Image::createAllocatedImage(
        _engine, "ViewpointAnalyzer depth image", DepthFormat, _extent,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT, 1, false, 1, VK_SAMPLE_COUNT_1_BIT
    ));
}

void ViewpointAnalyzer::createPipeline()
{
    vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addPushConstantRange(0, sizeof(PushConstantData), VK_SHADER_STAGE_VERTEX_BIT);
    _pipelineLayout = layoutFactory.build("ViewpointAnalyzer::PipelineLayout");

    vulkan::factory::GraphicsPipeline pipelineFactory(_engine);
    pipelineFactory.addShader("pick_selection.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    pipelineFactory.addShader("pick_selection.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    pipelineFactory.setInputState<scene::RenderMesh>();
    pipelineFactory.setColorAttachmentFormat(IdentifierFormat);
    pipelineFactory.setDepthFormat(DepthFormat);
    pipelineFactory.enableDepthtest(true, VK_COMPARE_OP_LESS);
    pipelineFactory.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineFactory.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineFactory.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    _pipeline = pipelineFactory.build(_pipelineLayout, "ViewpointAnalyzer::Pipeline");
}

void ViewpointAnalyzer::cleanupImages()
{
    _identifierImage.reset();
    _depthImage.reset();
    _extent = { 0, 0 };
}

}
