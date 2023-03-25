
#include <bg2e/render/vulkan/ImmediateCommandBuffer.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

ImmediateCommandBuffer::ImmediateCommandBuffer(vk::Device device, vk::CommandPool pool, vk::Queue queue, vk::CommandBuffer cmdBuffer)
:_device{device}
,_commandPool{pool}
,_queue{queue}
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
    vk::CommandBuffer cmd = _commandBuffer;
    
    vk::CommandBufferBeginInfo cmdBeginInfo;
    cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    cmd.begin(cmdBeginInfo);
    
    func(cmd);
    
    cmd.end();
    
    vk::SubmitInfo submit;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    if (_queue.submit(1, &submit, _commandFence) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Could not submit one time execution command buffer");
    }
    
    if (_device.waitForFences(1, &_commandFence, true, UINT64_MAX) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Error waiting for fences");
    }
    if (_device.resetFences(1, &_commandFence) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Could not reset one time execution command fence");
    }
    
    _device.resetCommandPool(_commandPool);
}

}
}
}
