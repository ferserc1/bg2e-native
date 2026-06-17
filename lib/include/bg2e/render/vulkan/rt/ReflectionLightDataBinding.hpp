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

#include <bg2e/render/vulkan/PipelineDataBinding.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/base/Light.hpp>

#include <vector>

namespace bg2e {
namespace render {
namespace vulkan {
namespace rt {

// Dedicated data binding that exposes, to the ray traced reflection closest-hit
// shader, only the lights flagged as "affects reflections". It is kept separate
// from the general deferred light binding so the reflection shader iterates over
// the smallest possible light set, and so it is only built when ray tracing is
// available.
class BG2E_API ReflectionLightDataBinding : public PipelineDataBinding {
public:
    static constexpr uint32_t MAX_LIGHTS = 8;

    ReflectionLightDataBinding(bg2e::render::Engine * engine) : PipelineDataBinding(engine) {}

    void initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator * frameAllocator) override;

    VkDescriptorSetLayout createLayout(
        VkShaderStageFlags shaderStages = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
    ) override;

    VkDescriptorSet newDescriptorSet(
        bg2e::render::vulkan::FrameResources & frameResources,
        const std::vector<base::LightData> & lights
    );
};

}
}
}
}
