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

#include <bg2e/render/vulkan/DescriptorSet.hpp>
#include <bg2e/render/vulkan/Buffer.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/Engine.hpp>

#include <memory>
#include <functional>

namespace bg2e {
namespace render {
namespace vulkan {
namespace macros {

// This function is usefull when we have a descriptor set that only contains
// one uniform buffer. This function will create the buffer, upload the data,
// create a descriptor set and update it in one single call.
// If your descriptor set contains more elements, you must to use other
// alternatives.
// The DescriptorSet memory created in this function is managed by the
// frame resources. Do not try to manage this descriptor set with any
// kind of smart pointer, the heap memory used by the descriptor set object
// will be automatically released when the frame is done.
template <typename T>
DescriptorSet* uniformBufferDescriptorSet(
    Engine * engine,
    FrameResources& frameResources,
    VkDescriptorSetLayout descriptorSetLayout,
    const T& data, uint32_t currentFrame
) {
     auto uniformBuffer = Buffer::createAllocatedBuffer(
        engine,
        sizeof(T),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY
    );
    
    auto dataPtr = reinterpret_cast<T*>(uniformBuffer->allocatedData());
    *dataPtr = data;
    
    auto descriptorSet = frameResources.newDescriptorSet(descriptorSetLayout);
    descriptorSet->updateBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniformBuffer, sizeof(T), 0);
    frameResources.cleanupManager.push([&, uniformBuffer](VkDevice dev) {
        uniformBuffer->cleanup();
        delete uniformBuffer;
    });
    return descriptorSet;
}

}
}
}
}
