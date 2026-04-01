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

#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/Command.hpp>
#include <bg2e/render/vulkan/CleanupManager.hpp>
#include <bg2e/render/vulkan/Device.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

class DescriptorSetAllocator;
class DescriptorSet;

struct BG2E_API FrameResources {
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkSemaphore swapchainSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderSemaphore = VK_NULL_HANDLE;
    VkFence frameFence = VK_NULL_HANDLE;
    CleanupManager cleanupManager;
    DescriptorSetAllocator* descriptorAllocator = nullptr;

    void init(const Device& device, Command* command);

    // Remove temporary resources used by this frame
    void flushFrameData();

    // Remove all the resources used by the frame
    void cleanup();
    
    // A simple way to allocate a descriptor set. The pointer to this
    // DescriptorSet object will be automatically released after the
    // frame is done. Important: do not store this pointer in any kind
    // of smart pointer
    DescriptorSet* newDescriptorSet(VkDescriptorSetLayout);

private:
    const Device * _device;
    Command* _command = nullptr;
};


}
}
}

