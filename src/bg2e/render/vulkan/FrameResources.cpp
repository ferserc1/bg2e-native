#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/vulkan/DescriptorSetAllocator.hpp>
#include <bg2e/render/vulkan/Info.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

void FrameResources::init(const Device& device, Command* command)
{
    _device = &device;
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
}

void FrameResources::flushFrameData()
{
    descriptorAllocator->clearDescriptors();
    cleanupManager.flush(*_device);
}

void FrameResources::cleanup()
{
    descriptorAllocator->clearDescriptors();
    descriptorAllocator->destroy();
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


}
}
}

