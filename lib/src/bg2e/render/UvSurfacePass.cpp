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

#include "UvSurfacePass.hpp"

#include <bg2e/render/Engine.hpp>
#include <bg2e/render/vulkan/factory/GraphicsPipeline.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/vulkan/macros/graphics.hpp>
#include <bg2e/render/vulkan/Buffer.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/scene/Drawable.hpp>

#include <cstring>
#include <stdexcept>

namespace bg2e::render {

namespace {

GBufferManager::Configuration uvSurfaceConfiguration()
{
    GBufferManager::Configuration configuration;
    configuration.colorFormats = {
        VK_FORMAT_R32G32B32A32_SFLOAT, // world position
        VK_FORMAT_R16G16B16A16_SFLOAT, // signed world normal
        VK_FORMAT_R32_UINT,            // zero-based submesh/material identity
        VK_FORMAT_R32_UINT             // valid texel mask
    };
    configuration.depthFormat = VK_FORMAT_UNDEFINED;
    return configuration;
}

}

UvSurfacePass::UvSurfacePass(Engine* engine, VkExtent2D extent)
    : _engine(engine), _extent(extent)
{
    if (!_engine)
    {
        throw std::invalid_argument("UvSurfacePass: engine must not be null");
    }
    if (_extent.width == 0 || _extent.height == 0)
    {
        throw std::invalid_argument("UvSurfacePass: extent must be non-zero");
    }

    _managers.reserve(_engine->numImages());
    for (uint32_t slot = 0; slot < _engine->numImages(); ++slot)
    {
        auto manager = std::make_unique<GBufferManager>(_engine, uvSurfaceConfiguration());
        manager->build(_extent);
        _managers.push_back(std::move(manager));
    }
    createPipeline();
}

UvSurfacePass::~UvSurfacePass()
{
    if (_pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(_engine->device().handle(), _pipeline, nullptr);
    }
    if (_pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(_engine->device().handle(), _pipelineLayout, nullptr);
    }
}

void UvSurfacePass::createPipeline()
{
    vulkan::factory::PipelineLayout layoutFactory(_engine);
    layoutFactory.addPushConstantRange(0, sizeof(glm::mat4) + sizeof(uint32_t), VK_SHADER_STAGE_VERTEX_BIT);
    _pipelineLayout = layoutFactory.build("UvSurfacePass::PipelineLayout");

    vulkan::factory::GraphicsPipeline pipelineFactory(_engine);
    pipelineFactory.setInputState<scene::Drawable>();
    pipelineFactory.disableMultisample();
    pipelineFactory.disableDepthtest();
    pipelineFactory.setColorAttachmentFormat(_managers.front()->formats());
    pipelineFactory.setDepthFormat(VK_FORMAT_UNDEFINED);
    pipelineFactory.addShader("uv_surface.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    pipelineFactory.addShader("uv_surface.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    _pipeline = pipelineFactory.build(_pipelineLayout, "UvSurfacePass::Pipeline");
}

void UvSurfacePass::record(VkCommandBuffer cmd,
                           const scene::Drawable& drawable,
                           const glm::mat4& nodeWorld,
                           uint32_t frameSlot)
{
    auto& target = manager(frameSlot);
    target.beginRender(cmd);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
    vulkan::macros::cmdSetDefaultViewportAndScissor(cmd, _extent);

    auto renderMesh = const_cast<scene::Drawable&>(drawable).renderMesh();
    for (uint32_t submeshIndex = 0; submeshIndex < drawable.submeshesCount(); ++submeshIndex)
    {
        // Vulkan's negative-height viewport maps UV2 v=1 to the first (top) image row.
        const glm::mat4 objectToWorld = nodeWorld * drawable.transform() *
            drawable.localSubmeshTransform(submeshIndex);
        vkCmdPushConstants(cmd, _pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
            0, sizeof(glm::mat4), &objectToWorld);
        vkCmdPushConstants(cmd, _pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
            sizeof(glm::mat4), sizeof(uint32_t), &submeshIndex);
        renderMesh->drawSubmesh(cmd, submeshIndex);
    }

    vulkan::cmdEndRendering(cmd);
    target.transitionToShaderRead(cmd);
}

GBufferManager& UvSurfacePass::manager(uint32_t frameSlot)
{
    if (frameSlot >= _managers.size())
    {
        throw std::out_of_range("UvSurfacePass: frame slot is out of range");
    }
    return *_managers[frameSlot];
}

const GBufferManager& UvSurfacePass::manager(uint32_t frameSlot) const
{
    if (frameSlot >= _managers.size())
    {
        throw std::out_of_range("UvSurfacePass: frame slot is out of range");
    }
    return *_managers[frameSlot];
}

UvSurfacePass::Diagnostic UvSurfacePass::readDiagnostic(uint32_t frameSlot) const
{
    const auto& target = manager(frameSlot);
    Diagnostic result;
    result.extent = target.extent();

    const VkDeviceSize pixelCount = static_cast<VkDeviceSize>(result.extent.width) * result.extent.height;
    const VkDeviceSize imageBytes = pixelCount * sizeof(uint32_t);
    auto stagingBuffer = std::unique_ptr<vulkan::Buffer>(vulkan::Buffer::createAllocatedBuffer(
        _engine,
        imageBytes * 2,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_TO_CPU,
        "UV surface diagnostic readback"
    ));

    // Diagnostic readback is intentionally blocking and may be invoked only after
    // the frame containing record() has been submitted.
    _engine->device().waitIdle();
    _engine->command().immediateSubmit([&](VkCommandBuffer cmd) {
        const auto idImage = target.image(2);
        const auto maskImage = target.image(3);
        vulkan::Image::TransitionInfo toTransfer(
            VK_IMAGE_ASPECT_COLOR_BIT,
            0,
            VK_REMAINING_MIP_LEVELS,
            0,
            VK_REMAINING_ARRAY_LAYERS,
            VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            VK_ACCESS_2_TRANSFER_READ_BIT
        );
        vulkan::Image::cmdTransitionImage(cmd, idImage->handle(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, toTransfer);
        vulkan::Image::cmdTransitionImage(cmd, maskImage->handle(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, toTransfer);

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = { result.extent.width, result.extent.height, 1 };
        vkCmdCopyImageToBuffer(cmd, idImage->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            stagingBuffer->handle(), 1, &region);

        region.bufferOffset = imageBytes;
        vkCmdCopyImageToBuffer(cmd, maskImage->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            stagingBuffer->handle(), 1, &region);

        vulkan::Image::TransitionInfo toSampling(
            VK_IMAGE_ASPECT_COLOR_BIT,
            0,
            VK_REMAINING_MIP_LEVELS,
            0,
            VK_REMAINING_ARRAY_LAYERS,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            VK_ACCESS_2_TRANSFER_READ_BIT,
            VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            VK_ACCESS_2_SHADER_SAMPLED_READ_BIT
        );
        vulkan::Image::cmdTransitionImage(cmd, idImage->handle(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, toSampling);
        vulkan::Image::cmdTransitionImage(cmd, maskImage->handle(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, toSampling);
    });

    VK_ASSERT(vmaInvalidateAllocation(_engine->allocator(), stagingBuffer->allocation(), 0, VK_WHOLE_SIZE));
    const auto* mapped = static_cast<const uint32_t*>(stagingBuffer->allocatedData());
    result.submeshIndices.assign(mapped, mapped + pixelCount);
    result.validTexels.assign(mapped + pixelCount, mapped + pixelCount * 2);
    return result;
}

}
