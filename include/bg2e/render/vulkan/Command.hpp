
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/Device.hpp>
#include <functional>


namespace bg2e {
namespace render {

class Vulkan;

namespace vulkan {

class BG2E_API Command {
public:
    void init(Vulkan* vulkanData);
    
    VkCommandPool createCommandPool(VkCommandPoolCreateFlags flags);
    
    VkCommandBuffer allocateCommandBuffer(VkCommandPool pool, uint32_t count = 1);
    
    void destroyComandPool(VkCommandPool pool);
    
    VkDevice device() const;
    inline VkQueue graphicsQueue() const { return _graphicsQueue; }
    inline uint32_t graphicsQueueFamily() const { return _graphicsQueueFamily; }

    void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

protected:
    VkQueue _graphicsQueue;
    uint32_t _graphicsQueueFamily;

    VkCommandPool _immediateCmdPool;
    VkCommandBuffer _immediateCmdBuffer;
    VkFence _immediateCmdFence;

private:
    Vulkan* _vulkan = nullptr;
};

}
}
}
