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

#include <bg2e/render/vulkan/rt/ReflectionLightDataBinding.hpp>
#include <bg2e/render/vulkan/macros/frame_resources.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>

namespace bg2e {
namespace render {
namespace vulkan {
namespace rt {

void ReflectionLightDataBinding::initFrameResources(DescriptorSetAllocator * frameAllocator)
{
    frameAllocator->requirePoolSizeRatio(1, {
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_LIGHTS }
    });
}

VkDescriptorSetLayout ReflectionLightDataBinding::createLayout(VkShaderStageFlags shaderStages)
{
    if (_layout == VK_NULL_HANDLE)
    {
        factory::DescriptorSetLayout dsFactory;
        dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        _layout = dsFactory.build(_engine->device().handle(), shaderStages);
    }
    return _layout;
}

VkDescriptorSet ReflectionLightDataBinding::newDescriptorSet(
    FrameResources & frameResources,
    const std::vector<base::LightData> & lights
) {
    if (_layout == VK_NULL_HANDLE)
    {
        throw std::runtime_error("ReflectionLightDataBinding::newDescriptorSet() - The descriptor set layout is not created");
    }

    // A storage buffer cannot be empty. When there are no reflection lights we
    // still allocate a single disabled light: the shader receives a light count
    // of zero and never reads it, but the descriptor stays valid.
    Buffer * lightBuffer = nullptr;
    size_t bufferLightCount = lights.size();
    if (bufferLightCount == 0)
    {
        std::vector<base::LightData> dummyLights {
            {
                .position = glm::vec3(0.0f, 0.0f, 0.0f),
                .intensity = 0.0f,
                .color = base::Color::Black(),
                .direction = glm::vec3(1.0f, 0.0f, 0.0f),
                .type = base::Light::LightType::TypeDisabled,
                .spotAngle = 0.0f,
                .spotCutoff = 0.0f,
                .castShadows = 0,
                .sourceSize = 0.0f,
                .shadowSamples = 0,
                .affectsReflections = 0
            }
        };
        lightBuffer = macros::createBuffer(
            _engine, frameResources, dummyLights,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY,
            "ReflectionLightDataBinding: light data SSBO"
        );
        bufferLightCount = 1;
    }
    else
    {
        lightBuffer = macros::createBuffer(
            _engine, frameResources, lights,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY,
            "ReflectionLightDataBinding: light data SSBO"
        );
    }

    auto ds = frameResources.newDescriptorSet(_layout);
    ds->beginUpdate();
        ds->addBuffer(
            0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            lightBuffer, sizeof(base::LightData) * bufferLightCount, 0
        );
    ds->endUpdate();
    return ds->descriptorSet();
}

}
}
}
}
