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
#include <bg2e/base/Light.hpp>
#include <glm/glm.hpp>

#include <vector>

#define BG2E_MAX_FORWARD_LIGHTS 8

namespace bg2e {
namespace scene {
namespace vk {

class BG2E_API LightDataBinding : public bg2e::render::vulkan::PipelineDataBinding {
public:
    struct LightData {
        glm::vec3 position;
        float intensity;
        base::Color color;
        glm::vec3 direction;
        base::Light::LightType type;
        float spotAngle;
        float spotCutoff;

        glm::vec2 padding;
    };

    struct LightUniforms {
        LightData lights[BG2E_MAX_FORWARD_LIGHTS];
        uint32_t lightCount = 0;
        glm::vec3 padding;
    };

    LightDataBinding(bg2e::render::Engine * engine) : PipelineDataBinding(engine) {}

    void initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator * frameAllocator) override;

    VkDescriptorSetLayout createLayout(VkShaderStageFlags shaderStages = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT) override;

    VkDescriptorSet newDescriptorSet(
        bg2e::render::vulkan::FrameResources & frameResources,
        const LightUniforms& lights
    );
};

}
}
}
