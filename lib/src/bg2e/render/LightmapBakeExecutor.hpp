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

#include <bg2e/base/Light.hpp>
#include <bg2e/render/LightmapBaker.hpp>
#include <bg2e/render/deferred/RTAmbientOcclusion.hpp>
#include <bg2e/render/deferred/RTGlobalIllumination.hpp>
#include <bg2e/render/vulkan/DescriptorSetAllocator.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/vulkan/rt/RayTracingScene.hpp>

#include <vector>

namespace bg2e::render {

class LightmapCompositionPass;

class LightmapBakeExecutor final {
  public:
    static void recordSample(
        LightmapBaker& baker,
        VkCommandBuffer commandBuffer,
        uint32_t frameNumber,
        uint32_t frameSlot,
        vulkan::FrameResources& frameResources,
        vulkan::DescriptorSetAllocator& descriptorAllocator,
        const vulkan::rt::RayTracingScene& rayTracingScene,
        deferred::RTAmbientOcclusion& rtaoPass,
        deferred::RTGlobalIllumination& rtgiPass,
        LightmapCompositionPass& compositionPass,
        vulkan::Image* irradianceMap,
        VkSampler irradianceSampler,
        const std::vector<base::LightData>& sceneLights);
};

}
