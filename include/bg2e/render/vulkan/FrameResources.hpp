#pragma once

#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/Command.hpp>
#include <bg2e/render/vulkan/CleanupManager.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

class DescriptorSetAllocator;

struct BG2E_API FrameResources {
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore swapchainSemaphore;
    VkSemaphore renderSemaphore;
    VkFence frameFence;
    CleanupManager cleanupManager;
    DescriptorSetAllocator* descriptorAllocator;

    void init(VkDevice device, Command* command);

    // Remove temporary resources used by this frame
    void flushFrameData();

    // Remove all the resources used by the frame
    void cleanup();

private:
    VkDevice _device;
    Command* _command;
};

constexpr unsigned int FRAME_OVERLAP = 2;

}
}
}

