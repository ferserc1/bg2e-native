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

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/vulkan/Image.hpp>

#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace bg2e {
namespace render {
namespace vulkan { class DescriptorSetAllocator; }
namespace deferred {

class BG2E_API SMAAProcessor {
public:
    SMAAProcessor(Engine* engine);
    ~SMAAProcessor();

    // Create pipelines, LUT textures, and per-frame images.
    // Call once during RendererDeferred::build().
    // outputFormat should match the swapchain color image format.
    void build(VkExtent2D extent, VkFormat outputFormat = VK_FORMAT_R8G8B8A8_UNORM);

    // Recreate per-frame images at new viewport size.
    // Pipelines and LUT textures are unchanged.
    // Call during RendererDeferred::resize().
    void resize(VkExtent2D newExtent);

    // Execute the three SMAA passes on the given command buffer.
    // inputImage must be in VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL on entry.
    // Returns a pointer to the per-frame outputImage (in SHADER_READ_ONLY_OPTIMAL layout).
    // The caller must transition inputImage back if needed after SMAA completes.
    const vulkan::Image* process(
        VkCommandBuffer cmd,
        uint32_t frameIndex,
        const vulkan::Image* inputImage
    );

    // Destroy all Vulkan resources.
    // Call during RendererDeferred::cleanup().
    void cleanup();

private:
    struct FrameData {
        std::unique_ptr<vulkan::Image> edgesImage;
        std::unique_ptr<vulkan::Image> blendWeightsImage;
        std::unique_ptr<vulkan::Image> outputImage;
    };

    Engine* _engine = nullptr;
    VkExtent2D _extent = {};
    VkFormat _outputFormat = VK_FORMAT_R8G8B8A8_UNORM;

    VkSampler _sampler = VK_NULL_HANDLE;

    // Persistent pipelines (created once, reused every frame)
    VkPipeline _edgeDetectionPipeline = VK_NULL_HANDLE;
    VkPipeline _blendWeightPipeline = VK_NULL_HANDLE;
    VkPipeline _neighborhoodBlendPipeline = VK_NULL_HANDLE;

    VkPipelineLayout _edgePipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout _blendPipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout _nblendPipelineLayout = VK_NULL_HANDLE;

    VkDescriptorSetLayout _edgeDSLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _blendWeightDSLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _neighborhoodBlendDSLayout = VK_NULL_HANDLE;

    // Static LUT textures (precomputed AreaTex/SearchTex from iryku_smaa)
    std::unique_ptr<vulkan::Image> _areaTexture;
    std::unique_ptr<vulkan::Image> _searchTexture;

    // Per-frame-in-flight resources
    std::vector<FrameData> _frames;

    // Internal helpers
    void loadLUTTextures();

    void createPipelines();
    void createFrameImages(VkExtent2D extent);
    void cleanupFrameImages();

    // Per-frame descriptor set allocators (one per in-flight frame)
    std::vector<vulkan::DescriptorSetAllocator*> _descriptorAllocators;

    void createDescriptorAllocators();

    void destroyAllocators();

    // Push constant struct (shared across all three passes)
    struct SMAAPushConstants {
        glm::vec2 texelSize;  // 1.0 / viewportSize
    };
};

}
}
}
