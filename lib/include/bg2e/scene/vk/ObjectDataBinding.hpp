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

#include <bg2e/scene/vk/SceneDataBinding.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/MaterialBase.hpp>
#include <bg2e/render/uniforms/materials.hpp>

namespace bg2e {
namespace scene {
namespace vk {

class BG2E_API ObjectDataBinding : public SceneDataBinding {
public:
    struct ObjectUniforms
    {
        glm::mat4 modelMatrix;
        
        render::uniforms::PBRMaterialData material;
    };

    ObjectDataBinding(bg2e::render::Engine * engine) :SceneDataBinding(engine) {}
    
    void initFrameResources(bg2e::render::vulkan::DescriptorSetAllocator * frameAllocator) override;
    
    // The invoker of this function is the owner of the Set Descriptor and shall be responsible for deleting it.
    VkDescriptorSetLayout createLayout() override;
    
    VkDescriptorSet newDescriptorSet(
        bg2e::render::vulkan::FrameResources & frameResources,
        bg2e::render::MaterialBase * material,
        const glm::mat4& modelMatrix
    );
};

}
}
}
