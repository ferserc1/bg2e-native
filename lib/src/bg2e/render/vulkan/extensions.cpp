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
        bg2e_log_debug << "\t" << fnName << bg2e_log_end;
    }
    return func;
}

void loadExtensions(VkInstance instance)
{
    bg2e_log_debug << "Loading extension functions:" << bg2e_log_end;
    cmdBeginRendering = loadExtension<PFN_vkCmdBeginRenderingKHR>(instance, "vkCmdBeginRenderingKHR");
    cmdEndRendering = loadExtension<PFN_vkCmdEndRenderingKHR>(instance, "vkCmdEndRenderingKHR");
    acquireNextImage = loadExtension<PFN_vkAcquireNextImageKHR>(instance, "vkAcquireNextImageKHR");
    queuePresent = loadExtension<PFN_vkQueuePresentKHR>(instance, "vkQueuePresentKHR");
    destroySwapchain = loadExtension<PFN_vkDestroySwapchainKHR>(instance, "vkDestroySwapchainKHR");
    destroySurface = loadExtension<PFN_vkDestroySurfaceKHR>(instance, "vkDestroySurfaceKHR");
    queueSubmit2 = loadExtension<PFN_vkQueueSubmit2KHR>(instance, "vkQueueSubmit2KHR");
    cmdPipelineBarrier2 = loadExtension<PFN_vkCmdPipelineBarrier2KHR>(instance, "vkCmdPipelineBarrier2KHR");
    cmdBlitImage2 = loadExtension<PFN_vkCmdBlitImage2KHR>(instance, "vkCmdBlitImage2KHR");
    
    PFN_vkGetDeviceProcAddr getDeviceProcAddr = (PFN_vkGetDeviceProcAddr) vkGetInstanceProcAddr(instance, "vkGetDeviceProcAddr");
    if (getDeviceProcAddr) {
        cmdBuildAccelerationStructures = (PFN_vkCmdBuildAccelerationStructuresKHR) getDeviceProcAddr(nullptr, "vkCmdBuildAccelerationStructuresKHR");
        cmdCopyAccelerationStructure = (PFN_vkCmdCopyAccelerationStructureKHR) getDeviceProcAddr(nullptr, "vkCmdCopyAccelerationStructureKHR");
        cmdCopyAccelerationStructureToMemory = (PFN_vkCmdCopyAccelerationStructureToMemoryKHR) getDeviceProcAddr(nullptr, "vkCmdCopyAccelerationStructureToMemoryKHR");
        cmdCopyMemoryToAccelerationStructure = (PFN_vkCmdCopyMemoryToAccelerationStructureKHR) getDeviceProcAddr(nullptr, "vkCmdCopyMemoryToAccelerationStructureKHR");
        cmdWriteAccelerationStructuresProperties = (PFN_vkCmdWriteAccelerationStructuresPropertiesKHR) getDeviceProcAddr(nullptr, "vkCmdWriteAccelerationStructuresPropertiesKHR");
        getAccelerationStructureBuildSizes = (PFN_vkGetAccelerationStructureBuildSizesKHR) getDeviceProcAddr(nullptr, "vkGetAccelerationStructureBuildSizesKHR");
        createRayTracingPipelines = (PFN_vkCreateRayTracingPipelinesKHR) getDeviceProcAddr(nullptr, "vkCreateRayTracingPipelinesKHR");
        cmdTraceRays = (PFN_vkCmdTraceRaysKHR) getDeviceProcAddr(nullptr, "vkCmdTraceRaysKHR");
        getRayTracingCaptureReplayShaderGroupHandles = (PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR) getDeviceProcAddr(nullptr, "vkGetRayTracingCaptureReplayShaderGroupHandlesKHR");
        createDeferredOperation = (PFN_vkCreateDeferredOperationKHR) getDeviceProcAddr(nullptr, "vkCreateDeferredOperationKHR");
        destroyDeferredOperation = (PFN_vkDestroyDeferredOperationKHR) getDeviceProcAddr(nullptr, "vkDestroyDeferredOperationKHR");
        getDeferredOperationResult = (PFN_vkGetDeferredOperationResultKHR) getDeviceProcAddr(nullptr, "vkGetDeferredOperationResultKHR");
        deferredOperationJoin = (PFN_vkDeferredOperationJoinKHR) getDeviceProcAddr(nullptr, "vkDeferredOperationJoinKHR");
    }
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

// VK_KHR_acceleration_structure
PFN_vkCmdBuildAccelerationStructuresKHR                cmdBuildAccelerationStructures;
PFN_vkCmdCopyAccelerationStructureKHR                  cmdCopyAccelerationStructure;
PFN_vkCmdCopyAccelerationStructureToMemoryKHR          cmdCopyAccelerationStructureToMemory;
PFN_vkCmdCopyMemoryToAccelerationStructureKHR          cmdCopyMemoryToAccelerationStructure;
PFN_vkCmdWriteAccelerationStructuresPropertiesKHR      cmdWriteAccelerationStructuresProperties;
PFN_vkGetAccelerationStructureBuildSizesKHR            getAccelerationStructureBuildSizes;

// VK_KHR_ray_tracing_pipeline
PFN_vkCreateRayTracingPipelinesKHR                     createRayTracingPipelines;
PFN_vkCmdTraceRaysKHR                                  cmdTraceRays;
PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR  getRayTracingCaptureReplayShaderGroupHandles;

// VK_KHR_deferred_host_operations
PFN_vkCreateDeferredOperationKHR                       createDeferredOperation;
PFN_vkDestroyDeferredOperationKHR                      destroyDeferredOperation;
PFN_vkGetDeferredOperationResultKHR                    getDeferredOperationResult;
PFN_vkDeferredOperationJoinKHR                         deferredOperationJoin;

}
}
}

