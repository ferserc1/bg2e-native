#pragma once

#include <bg2e/render/common.hpp>

#include <bg2e/render/Command.hpp>
#include <bg2e/render/CleanupManager.hpp>

namespace bg2e {
namespace render {

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
