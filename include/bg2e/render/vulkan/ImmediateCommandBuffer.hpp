

#ifndef bg2e_render_vulkan_immediatecommandbuffer_hpp
#define bg2e_render_vulkan_immediatecommandbuffer_hpp

#include <vulkan/vulkan.hpp>

#include <functional>

namespace bg2e {
namespace render {
namespace vulkan {

// This object contains all the necesary resources to execute a command buffer
// using a lambda closure. You can create an ImmediateCommandBuffer object with
// all the resources needed to execute the command buffer (commandBuffer, queue,
// fences, device, etc) and call the execute function with a lambda closure.
// You can also pass the ImmediateCommandBuffer object to other functions that
// need to execute a command buffer.
class ImmediateCommandBuffer
{
public:
    ImmediateCommandBuffer(vk::Device device, vk::CommandPool pool, vk::Queue queue, vk::CommandBuffer cmdBuffer);
    ~ImmediateCommandBuffer();
    
    void execute(std::function<void(vk::CommandBuffer)>&& func);
    
protected:
    vk::Device _device;
    vk::CommandPool _commandPool;
    vk::CommandBuffer _commandBuffer;
    vk::Queue _queue;
    vk::Fence _commandFence;
};

}
}
}

#endif

