#pragma once

#include <bg2e/base/PlatformTools.hpp>
#include <vulkan/vulkan.h>

namespace bg2e {
namespace render {
namespace vulkan {

void loadExtensions(VkInstance instance);

// VK_KHR_dynamic_rendering
extern PFN_vkCmdBeginRenderingKHR      cmdBeginRendering;
extern PFN_vkCmdEndRenderingKHR        cmdEndRendering;

// VK_KHR_swapchain
extern PFN_vkAcquireNextImageKHR       acquireNextImage;
extern PFN_vkQueuePresentKHR           queuePresent;
extern PFN_vkDestroySwapchainKHR       destroySwapchain;

extern PFN_vkDestroySurfaceKHR         destroySurface;

// VK_KHR_synchronization2
extern PFN_vkQueueSubmit2KHR           queueSubmit2;

extern PFN_vkCmdPipelineBarrier2       cmdPipelineBarrier2;

// VK_KHR_copy_commands2
extern PFN_vkCmdBlitImage2             cmdBlitImage2;

}
}
}

