/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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

