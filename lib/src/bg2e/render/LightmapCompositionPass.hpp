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
#include <bg2e/render/LightmapSettings.hpp>
#include <bg2e/render/gbuffer/GBufferManager.hpp>
#include <bg2e/render/vulkan/Image.hpp>

namespace bg2e::render {

class LightmapCompositionPass
{
public:
    explicit LightmapCompositionPass(Engine* engine);
    ~LightmapCompositionPass();

    LightmapCompositionPass(const LightmapCompositionPass&) = delete;
    LightmapCompositionPass& operator=(const LightmapCompositionPass&) = delete;

    void render(
        VkCommandBuffer cmd,
        vulkan::DescriptorSetAllocator& descriptorAllocator,
        const GBufferManager& uvSurface,
        const vulkan::Image& aoImage,
        const vulkan::Image& giImage,
        const vulkan::Image& shadowImage,
        vulkan::Image& output,
        LightmapMode mode,
        bool useShadows);

    void cleanup();

private:
    Engine* _engine = nullptr;
    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;
    VkSampler _colorSampler = VK_NULL_HANDLE;
    VkSampler _maskSampler = VK_NULL_HANDLE;

    struct PushConstants {
        uint32_t mode;
        uint32_t useShadows;
    };

    void createPipeline();
};

}
