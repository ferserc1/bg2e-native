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

#include <bg2e/render/Engine.hpp>
#include <bg2e/render/gbuffer/GBufferManager.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/render/vulkan/rt/RTMaterialDataBinding.hpp>
#include <bg2e/render/vulkan/rt/ReflectionLightDataBinding.hpp>

#include <vector>

namespace bg2e::render::deferred {

class RTShadowVisibilityPass
{
public:
    RTShadowVisibilityPass(
        Engine* engine,
        vulkan::rt::RTMaterialDataBinding* materialDataBinding,
        vulkan::rt::ReflectionLightDataBinding* lightDataBinding);
    ~RTShadowVisibilityPass();

    RTShadowVisibilityPass(const RTShadowVisibilityPass&) = delete;
    RTShadowVisibilityPass& operator=(const RTShadowVisibilityPass&) = delete;

    void renderUv(
        VkCommandBuffer cmd,
        uint32_t currentFrame,
        vulkan::FrameResources& frameResources,
        vulkan::DescriptorSetAllocator& descriptorAllocator,
        const GBufferManager& uvSurface,
        const vulkan::rt::RayTracingScene& rayTracingScene,
        const std::vector<base::LightData>& lights,
        vulkan::Image& shadowOutput);

    void clearNeutral(VkCommandBuffer cmd, vulkan::Image& shadowOutput);
    void cleanup();

private:
    Engine* _engine = nullptr;
    vulkan::rt::RTMaterialDataBinding* _materialDataBinding = nullptr;
    vulkan::rt::ReflectionLightDataBinding* _lightDataBinding = nullptr;
    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _surfaceLayout = VK_NULL_HANDLE;
    VkSampler _surfaceSampler = VK_NULL_HANDLE;
    VkSampler _maskSampler = VK_NULL_HANDLE;

    struct PushConstants {
        uint32_t lightCount;
    };

    void createPipeline();
};

}
