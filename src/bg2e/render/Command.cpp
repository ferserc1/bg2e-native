
#include <bg2e/render/Command.hpp>
#include <bg2e/render/Info.hpp>
#include <bg2e/render/common.hpp>
#include <bg2e/render/Vulkan.hpp>


namespace bg2e {
namespace render {

void Command::init(Vulkan *vulkan, vkb::Device *bDevice)
{
    _vulkan = vulkan;
    
    _graphicsQueue = bDevice->get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamily = bDevice->get_queue_index(vkb::QueueType::graphics).value();

    auto cmdPoolInfo = Info::commandPoolCreateInfo(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_ASSERT(vkCreateCommandPool(_vulkan->device(), &cmdPoolInfo, nullptr, &_immediateCmdPool));
    
    auto cmdAllocInfo = Info::commandBufferAllocateInfo(_immediateCmdPool);
    VK_ASSERT(vkAllocateCommandBuffers(_vulkan->device(), &cmdAllocInfo, &_immediateCmdBuffer));
    
    auto fenceInfo = Info::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_ASSERT(vkCreateFence(_vulkan->device(), &fenceInfo, nullptr, &_immediateCmdFence));
    _vulkan->cleanupManager().push([&](VkDevice) {
        vkDestroyCommandPool(_vulkan->device(), _immediateCmdPool, nullptr);
        vkDestroyFence(_vulkan->device(), _immediateCmdFence, nullptr);
    });
}

VkCommandPool Command::createCommandPool(VkCommandPoolCreateFlags flags)
{
    VkCommandPool pool;
    auto poolInfo = Info::commandPoolCreateInfo(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    
    VK_ASSERT(vkCreateCommandPool(_vulkan->device(), &poolInfo, nullptr, &pool));
    
    return pool;
}
    
VkCommandBuffer Command::allocateCommandBuffer(VkCommandPool pool, uint32_t count)
{
    auto allocInfo = Info::commandBufferAllocateInfo(pool, count);
    VkCommandBuffer buffer;
    
    VK_ASSERT(vkAllocateCommandBuffers(_vulkan->device(), &allocInfo, &buffer));
    
    return buffer;
}

void Command::destroyComandPool(VkCommandPool pool)
{
    vkDestroyCommandPool(_vulkan->device(), pool, nullptr);
}

VkDevice Command::device() const
{
    return _vulkan->device();
}

void Command::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
{
    VK_ASSERT(vkResetFences(_vulkan->device(), 1, &_immediateCmdFence));
	VK_ASSERT(vkResetCommandBuffer(_immediateCmdBuffer, 0));

	VkCommandBufferBeginInfo cmdBeginInfo = Info::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_ASSERT(vkBeginCommandBuffer(_immediateCmdBuffer, &cmdBeginInfo));

	function(_immediateCmdBuffer);

	VK_ASSERT(vkEndCommandBuffer(_immediateCmdBuffer));

	auto cmdInfo = Info::commandBufferSubmitInfo(_immediateCmdBuffer);
    auto submit = Info::submitInfo(&cmdInfo, nullptr, nullptr);

    queueSubmit2(_graphicsQueue, 1, &submit, _immediateCmdFence);

	VK_ASSERT(vkWaitForFences(_vulkan->device(), 1, &_immediateCmdFence, true, 9999999999));
}

}
}
