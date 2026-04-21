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

#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/vulkan/DescriptorSetAllocator.hpp>
#include <bg2e/render/vulkan/Info.hpp>
#include <bg2e/render/Engine.hpp>
#include <bg2e/render/vulkan/rt/RayTracingScene.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

void FrameResources::init(bg2e::render::Engine * engine, Command* command)
{
    _engine = engine;
    _device = &_engine->device();
    _command = command;

    // Command pool and command buffer
    commandPool = command->createCommandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffer = command->allocateCommandBuffer(commandPool, 1);

    // Synchonization structures
    auto fenceInfo = Info::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_ASSERT(vkCreateFence(_command->device(), &fenceInfo, nullptr, &frameFence));

    auto semaphoreInfo = Info::semaphoreCreateInfo();
    VK_ASSERT(vkCreateSemaphore(_command->device(), &semaphoreInfo, nullptr, &swapchainSemaphore));
    VK_ASSERT(vkCreateSemaphore(_command->device(), &semaphoreInfo, nullptr, &renderSemaphore));

    descriptorAllocator = new DescriptorSetAllocator();

    rayTracingScene = new rt::RayTracingScene(_engine);
}

void FrameResources::flushFrameData()
{
    descriptorAllocator->clearDescriptors();
    cleanupManager.flush(*_device);
}

void FrameResources::cleanup()
{
    if (rayTracingScene)
    {
        delete rayTracingScene;
    }

    if (descriptorAllocator)
    {
        delete descriptorAllocator;

        // Destroy command pool
        _command->destroyComandPool(commandPool);

        // Destroy synchronization structures
        vkDestroyFence(_command->device(), frameFence, nullptr);
        vkDestroySemaphore(_command->device(), swapchainSemaphore, nullptr);
        vkDestroySemaphore(_command->device(), renderSemaphore, nullptr);
        
        // Destroy frame cleanup manager
        cleanupManager.flush(*_device);
    }
    
    _device = nullptr;
    _command = nullptr;
    descriptorAllocator = nullptr;
    commandPool = VK_NULL_HANDLE;
    commandBuffer = VK_NULL_HANDLE;
    swapchainSemaphore = VK_NULL_HANDLE;
    renderSemaphore = VK_NULL_HANDLE;
    frameFence = VK_NULL_HANDLE;
}

DescriptorSet* FrameResources::newDescriptorSet(VkDescriptorSetLayout layout)
{
    auto ds = descriptorAllocator->allocate(layout);
    cleanupManager.push([ds](VkDevice) {
        delete ds;
    });
    return ds;
}


}
}
}

