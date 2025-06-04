
#include <bg2e/render/vulkan/Command.hpp>
#include <bg2e/render/vulkan/Info.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/Engine.hpp>


namespace bg2e {
namespace render {
namespace vulkan {

void Command::init(Engine *engine)
{
    _engine = engine;
    
    _graphicsQueue = engine->device().graphicsQueue();
    _graphicsQueueFamily = engine->device().graphicsFamily();

    auto cmdPoolInfo = Info::commandPoolCreateInfo(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_ASSERT(vkCreateCommandPool(_engine->device().handle(), &cmdPoolInfo, nullptr, &_immediateCmdPool));
    
    auto cmdAllocInfo = Info::commandBufferAllocateInfo(_immediateCmdPool);
    VK_ASSERT(vkAllocateCommandBuffers(_engine->device().handle(), &cmdAllocInfo, &_immediateCmdBuffer));
    
    auto fenceInfo = Info::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_ASSERT(vkCreateFence(_engine->device().handle(), &fenceInfo, nullptr, &_immediateCmdFence));
    _engine->cleanupManager().push([&](VkDevice) {
        vkDestroyCommandPool(_engine->device().handle(), _immediateCmdPool, nullptr);
        vkDestroyFence(_engine->device().handle(), _immediateCmdFence, nullptr);
    });
}

VkCommandPool Command::createCommandPool(VkCommandPoolCreateFlags flags)
{
    VkCommandPool pool;
    auto poolInfo = Info::commandPoolCreateInfo(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    
    VK_ASSERT(vkCreateCommandPool(_engine->device().handle(), &poolInfo, nullptr, &pool));
    
    return pool;
}
    
VkCommandBuffer Command::allocateCommandBuffer(VkCommandPool pool, uint32_t count)
{
    auto allocInfo = Info::commandBufferAllocateInfo(pool, count);
    VkCommandBuffer buffer;
    
    VK_ASSERT(vkAllocateCommandBuffers(_engine->device().handle(), &allocInfo, &buffer));
    
    return buffer;
}

void Command::destroyComandPool(VkCommandPool pool)
{
    vkDestroyCommandPool(_engine->device().handle(), pool, nullptr);
}

VkDevice Command::device() const
{
    return _engine->device().handle();
}

void Command::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
{
    VK_ASSERT(vkResetFences(_engine->device().handle(), 1, &_immediateCmdFence));
	VK_ASSERT(vkResetCommandBuffer(_immediateCmdBuffer, 0));

	VkCommandBufferBeginInfo cmdBeginInfo = Info::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_ASSERT(vkBeginCommandBuffer(_immediateCmdBuffer, &cmdBeginInfo));

	function(_immediateCmdBuffer);

	VK_ASSERT(vkEndCommandBuffer(_immediateCmdBuffer));

	auto cmdInfo = Info::commandBufferSubmitInfo(_immediateCmdBuffer);
    auto submit = Info::submitInfo(&cmdInfo, nullptr, nullptr);

    queueSubmit2(_graphicsQueue, 1, &submit, _immediateCmdFence);

	VK_ASSERT(vkWaitForFences(_engine->device().handle(), 1, &_immediateCmdFence, true, 9999999999));
}

}
}
}
