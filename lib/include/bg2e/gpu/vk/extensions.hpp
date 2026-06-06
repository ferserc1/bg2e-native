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
namespace gpu {
namespace vk {

class PhysicalDevice;

void loadInstanceExtensions(VkInstance instance, bool offscreen);

void loadDeviceExtensions(const bg2e::gpu::vk::PhysicalDevice& physDev, VkDevice device, bool offscreen);

// Functions with _OPT suffix are optional and may be nullptr if the hardware
// or the layer configuration does not support it.
// Allways check that _OPT function pointers are valid before use them

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

// VK_KHR_acceleration_structure
extern BG2E_API PFN_vkCmdBuildAccelerationStructuresKHR                cmdBuildAccelerationStructures;
extern BG2E_API PFN_vkCmdCopyAccelerationStructureKHR                  cmdCopyAccelerationStructure;
extern BG2E_API PFN_vkCmdCopyAccelerationStructureToMemoryKHR          cmdCopyAccelerationStructureToMemory;
extern BG2E_API PFN_vkCmdCopyMemoryToAccelerationStructureKHR          cmdCopyMemoryToAccelerationStructure;
extern BG2E_API PFN_vkCmdWriteAccelerationStructuresPropertiesKHR      cmdWriteAccelerationStructuresProperties;
extern BG2E_API PFN_vkGetAccelerationStructureBuildSizesKHR            getAccelerationStructureBuildSizes;
extern BG2E_API PFN_vkCreateAccelerationStructureKHR                   createAccelerationStructure;
extern BG2E_API PFN_vkDestroyAccelerationStructureKHR                  destroyAccelerationStructure;
extern BG2E_API PFN_vkGetAccelerationStructureDeviceAddressKHR         getAccelerationStructureDeviceAddress;

// VK_KHR_ray_tracing_pipeline
extern BG2E_API PFN_vkCreateRayTracingPipelinesKHR                     createRayTracingPipelines;
extern BG2E_API PFN_vkCmdTraceRaysKHR                                  cmdTraceRays;
extern BG2E_API PFN_vkGetRayTracingShaderGroupHandlesKHR               getRayTracingShaderGroupHandles;
extern BG2E_API PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR  getRayTracingCaptureReplayShaderGroupHandles;

// VK_KHR_deferred_host_operations
extern BG2E_API PFN_vkCreateDeferredOperationKHR                       createDeferredOperation;
extern BG2E_API PFN_vkDestroyDeferredOperationKHR                      destroyDeferredOperation;
extern BG2E_API PFN_vkGetDeferredOperationResultKHR                    getDeferredOperationResult;
extern BG2E_API PFN_vkDeferredOperationJoinKHR                         deferredOperationJoin;

// VK_EXT_DEBUG_UTILS_EXTENSION_NAME
extern BG2E_API PFN_vkSetDebugUtilsObjectNameEXT                       setDebugUtilsObjectName;

}
}
}
