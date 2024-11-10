#include <bg2e/render/vulkan/extensions.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/base/Log.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

template <class T>
T loadExtension(VkInstance instance, const char* fnName)
{
    auto func = (T) vkGetInstanceProcAddr(instance, fnName);
    if (func == nullptr)
    {
        throw std::runtime_error(std::string("Error loading extension: ") + fnName);
    }
    if (base::Log::isDebug())
    {
        bg2e_log_debug << "Extension function loaded: " << fnName << bg2e_log_end;
    }
    return func;
}

void loadExtensions(VkInstance instance)
{
    cmdBeginRendering = loadExtension<PFN_vkCmdBeginRenderingKHR>(instance, "vkCmdBeginRenderingKHR");
    cmdEndRendering = loadExtension<PFN_vkCmdEndRenderingKHR>(instance, "vkCmdEndRenderingKHR");
    acquireNextImage = loadExtension<PFN_vkAcquireNextImageKHR>(instance, "vkAcquireNextImageKHR");
    queuePresent = loadExtension<PFN_vkQueuePresentKHR>(instance, "vkQueuePresentKHR");
    destroySwapchain = loadExtension<PFN_vkDestroySwapchainKHR>(instance, "vkDestroySwapchainKHR");
    destroySurface = loadExtension<PFN_vkDestroySurfaceKHR>(instance, "vkDestroySurfaceKHR");
    queueSubmit2 = loadExtension<PFN_vkQueueSubmit2KHR>(instance, "vkQueueSubmit2KHR");
    cmdPipelineBarrier2 = loadExtension<PFN_vkCmdPipelineBarrier2>(instance, "vkCmdPipelineBarrier2");
    cmdBlitImage2 = loadExtension<PFN_vkCmdBlitImage2>(instance, "vkCmdBlitImage2");
}

// VK_KHR_dynamic_rendering
PFN_vkCmdBeginRenderingKHR      cmdBeginRendering;
PFN_vkCmdEndRenderingKHR        cmdEndRendering;

// VK_KHR_swapchain
PFN_vkAcquireNextImageKHR       acquireNextImage;
PFN_vkQueuePresentKHR           queuePresent;
PFN_vkDestroySwapchainKHR       destroySwapchain;

PFN_vkDestroySurfaceKHR         destroySurface;

// VK_KHR_synchronization2
PFN_vkQueueSubmit2KHR           queueSubmit2;

PFN_vkCmdPipelineBarrier2       cmdPipelineBarrier2;

// VK_KHR_copy_commands2
PFN_vkCmdBlitImage2             cmdBlitImage2;

}
}
}

