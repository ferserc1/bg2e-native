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
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore swapchainSemaphore;
    VkSemaphore renderSemaphore;
    VkFence frameFence;
    CleanupManager cleanupManager;
    DescriptorSetAllocator* descriptorAllocator;

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
    Command* _command;
};

constexpr unsigned int FRAME_OVERLAP = 2;

}
}
}

