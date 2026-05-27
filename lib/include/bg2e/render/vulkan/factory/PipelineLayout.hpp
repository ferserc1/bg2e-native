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
#include <vector>

namespace bg2e {
namespace render {
namespace vulkan {
namespace factory {

class BG2E_API PipelineLayout {
public:
    PipelineLayout(Engine * engine) :_engine{engine} {}

    inline void addPushConstantRange(VkPushConstantRange pushConstant) { _pushConstantRanges.push_back(pushConstant); }
    inline void addDescriptorSetLayout(VkDescriptorSetLayout dsLayout) { _descriptorSetLayouts.push_back(dsLayout); }
    inline void addPushConstantRange(uint32_t offset, size_t size, VkShaderStageFlags stageFlags)
    {
        addPushConstantRange({ .stageFlags = stageFlags, .offset = offset, .size = uint32_t(size) });
    }
    
    VkPipelineLayout build(const std::string& name);
    
    void reset();
    
protected:
    Engine * _engine;
    std::vector<VkPushConstantRange> _pushConstantRanges;
    std::vector<VkDescriptorSetLayout> _descriptorSetLayouts;
    
};

}
}
}
}
