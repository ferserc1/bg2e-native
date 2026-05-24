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
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/gbuffer/GBufferManager.hpp>

#include <vector>
#include <memory>

namespace bg2e {
namespace render {
namespace deferred {

class BG2E_API RTAmbientOcclusion {
public:
    RTAmbientOcclusion(Engine * engine);
    ~RTAmbientOcclusion();

    void build(VkExtent2D extent);
    void resize(VkExtent2D newExtent);
    void render(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        vulkan::FrameResources & frameResources,
        const GBufferManager * gbuffer,
        const glm::mat4 & inverseViewProjection
    );
    void cleanup();

    std::shared_ptr<vulkan::Image> aoImage(uint32_t frameIndex) const;
    VkSampler sampler() const;
    bool rtSupported() const;

private:
    Engine * _engine;
    VkExtent2D _extent;
    bool _rtSupported = false;

    std::vector<std::shared_ptr<vulkan::Image>> _aoImages;

    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _dsLayout = VK_NULL_HANDLE;
    VkSampler _sampler = VK_NULL_HANDLE;

    struct AOPushConstants {
        glm::mat4 inverseViewProjection;
        int sampleCount;
        int bounceCount;
        float radius;
        float bias;
        float falloff;
        float bounceAttenuation;
        uint32_t frameIndex;

        int padding0;
    };

    void createWhiteFallback();
    void createAOResources(VkExtent2D extent);
    void createPipeline();
    void cleanupImages();
};

}
}
}