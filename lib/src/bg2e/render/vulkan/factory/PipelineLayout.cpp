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

#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>
#include <bg2e/render/vulkan/Info.hpp>
#include <bg2e/render/vulkan/extensions.hpp>
#include <bg2e/base/Log.hpp>

namespace bg2e::render::vulkan::factory {

VkPipelineLayout PipelineLayout::build(const std::string & name)
{
    auto layoutInfo = Info::pipelineLayoutInfo();
    if (_pushConstantRanges.size() > 0)
    {
        layoutInfo.pPushConstantRanges = _pushConstantRanges.data();
        layoutInfo.pushConstantRangeCount = uint32_t(_pushConstantRanges.size());
    }
    
    if (_descriptorSetLayouts.size() > 0)
    {
        layoutInfo.pSetLayouts = _descriptorSetLayouts.data();
        layoutInfo.setLayoutCount = uint32_t(_descriptorSetLayouts.size());
    }

    VkPipelineLayout result = VK_NULL_HANDLE;
    VK_ASSERT(vkCreatePipelineLayout(_engine->device().handle(), &layoutInfo, nullptr, &result));

    if (base::Log::isDebug() || !name.empty() && setDebugUtilsObjectName != nullptr)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_PIPELINE_LAYOUT;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(result);
        nameInfo.pObjectName = name.c_str();

        setDebugUtilsObjectName(
            _engine->device().handle(),
            &nameInfo
        );
    }
    return result;
}

void PipelineLayout::reset()
{
    _pushConstantRanges.clear();
    _descriptorSetLayouts.clear();
}

}
