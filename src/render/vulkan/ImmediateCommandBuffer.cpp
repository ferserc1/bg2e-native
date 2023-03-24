
#include <bg2e/render/vulkan/ImmediateCommandBuffer.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

ImmediateCommandBuffer::ImmediateCommandBuffer(vk::Device device, vk::CommandPool pool, vk::CommandBuffer cmdBuffer)
:_device{device}
,_commandPool{pool}
,_commandBuffer{cmdBuffer}
{
    vk::FenceCreateInfo fenceInfo;
    _commandFence = _device.createFence(fenceInfo);
}

ImmediateCommandBuffer::~ImmediateCommandBuffer()
{
    _device.destroyFence(_commandFence, nullptr);
}

void ImmediateCommandBuffer::execute(std::function<void(vk::CommandBuffer)>&& func)
{
    /*

     TODO: Get the resources needed in this function from this object
     
     VkCommandBuffer cmd = _uploadContext._commandBuffer;
         
     VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
     VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
     
     function(cmd);
     
     VK_CHECK(vkEndCommandBuffer(cmd));
     
     VkSubmitInfo submit = vkinit::submit_info(&cmd);
     
     VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, _uploadContext._uploadFence));
     
     vkWaitForFences(_device, 1, &_uploadContext._uploadFence, true, 9999999999);
     vkResetFences(_device, 1, &_uploadContext._uploadFence);
     
     vkResetCommandPool(_device, _uploadContext._commandPool, 0);
 
 
     */
}

}
}
}
