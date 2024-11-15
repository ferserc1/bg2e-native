#pragma once

#include <bg2e/common.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <vulkan/vulkan.h>

namespace bg2e {
namespace render {
namespace vulkan {

void loadExtensions(VkInstance instance);

// VK_KHR_dynamic_rendering
extern BG2E_API PFN_vkCmdBeginRenderingKHR      cmdBeginRendering;
extern BG2E_API PFN_vkCmdEndRenderingKHR        cmdEndRendering;

// VK_KHR_swapchain
extern BG2E_API PFN_vkAcquireNextImageKHR       acquireNextImage;
extern BG2E_API PFN_vkQueuePresentKHR           queuePresent;
extern BG2E_API PFN_vkDestroySwapchainKHR       destroySwapchain;

extern BG2E_API PFN_vkDestroySurfaceKHR         destroySurface;

// VK_KHR_synchronization2
extern BG2E_API PFN_vkQueueSubmit2KHR           queueSubmit2;

extern BG2E_API PFN_vkCmdPipelineBarrier2       cmdPipelineBarrier2;

// VK_KHR_copy_commands2
extern BG2E_API PFN_vkCmdBlitImage2             cmdBlitImage2;

}
}
}

