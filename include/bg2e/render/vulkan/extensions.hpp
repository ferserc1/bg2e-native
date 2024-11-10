#pragma once

#include <bg2e/base/PlatformTools.hpp>
#include <vulkan/vulkan.h>
#include <VkBootstrap.h>

/*
    Currently MoltenVK does not support Vulkan API 1.3, so we have to use the KHR extension functions
    from API 1.2 on macOS. On Windows we have the problem that we have to load the KHR functions manually,
    or use API 1.3. A simpler solution to manage these problems is to create a facade for these functions,
    instead of using the Vulkan functions directly.

    This file provides that facade to make these calls seamless.
*/



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

