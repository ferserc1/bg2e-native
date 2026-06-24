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
#include <bg2e/render/vulkan/DescriptorSetAllocator.hpp>

#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace bg2e {
namespace render {
namespace deferred {

// Generates screen-space motion vectors (pixel units) from the current depth
// buffer plus the current and previous frame camera matrices.  Output is a
// per-frame RG16F image at render resolution suitable for FSR3 upscaling.
class BG2E_API MotionVectorGenerator {
public:
    explicit MotionVectorGenerator(Engine* engine);
    ~MotionVectorGenerator();

    MotionVectorGenerator(const MotionVectorGenerator&) = delete;
    MotionVectorGenerator& operator=(const MotionVectorGenerator&) = delete;

    void build(VkExtent2D renderExtent);
    void resize(VkExtent2D newExtent);

    // Transitions depthImage to SHADER_READ_ONLY_OPTIMAL, runs the compute
    // dispatch, and returns a pointer to the per-frame motion vector image
    // (left in GENERAL layout after write).
    const vulkan::Image* generate(
        VkCommandBuffer cmd,
        uint32_t frameIndex,
        const vulkan::Image* depthImage,
        const glm::mat4& currentInvViewProj,
        const glm::mat4& prevViewProj
    );

    void cleanup();

private:
    struct FrameData {
        std::unique_ptr<vulkan::Image> motionVectors;
    };

    struct PushConstants {
        glm::mat4 currentInvViewProj;
        glm::mat4 prevViewProj;
        glm::vec2 renderSize;
    };

    Engine*    _engine  = nullptr;
    VkExtent2D _extent  = {};

    VkPipeline            _pipeline       = VK_NULL_HANDLE;
    VkPipelineLayout      _pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _dsLayout       = VK_NULL_HANDLE;
    VkSampler             _sampler        = VK_NULL_HANDLE;

    std::vector<FrameData>                    _frames;
    std::vector<vulkan::DescriptorSetAllocator*> _descriptorAllocators;

    void createPipeline();
    void createFrameImages(VkExtent2D extent);
    void cleanupFrameImages();
    void createDescriptorAllocators();
    void destroyAllocators();
};

}
}
}
