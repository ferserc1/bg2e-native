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
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/render/vulkan/Buffer.hpp>
#include <bg2e/render/vulkan/factory/DescriptorSetLayout.hpp>
#include <bg2e/render/vulkan/factory/PipelineLayout.hpp>

#include <vector>

namespace bg2e {
namespace render {

class BG2E_API GPUProcess {
public:
    GPUProcess(Engine * engine);
    
    void addBinding(
        uint32_t binding,
        vulkan::Image * img,
        VkDescriptorType type,
        VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        VkImageLayout finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
    void addBinding(
        uint32_t binding,
        vulkan::Buffer * buffer,
        VkDescriptorType type,
        uint32_t bufferSize,
        uint32_t bufferOffset
    );
    
    void executeShader(const std::string& shaderFile, VkExtent2D imageExtent);
    
protected:
    Engine * _engine;

    union BindingPtr {
        vulkan::Image * image;
        vulkan::Buffer * buffer;
    };
    
    enum BindingType {
        TypeImage = 0,
        TypeBuffer = 1
    };
    
    struct BindingInfo {
        uint32_t index;
        BindingPtr data;
        BindingType type;
        VkDescriptorType descriptorType;
        VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        uint32_t bufferSize;
        uint32_t bufferOffset;
    };
    
    std::vector<BindingInfo> _bindings;
};

}
}
