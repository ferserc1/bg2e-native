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
#include <bg2e/render/vulkan/PhysicalDevice.hpp>
#include <bg2e/base/Log.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

template <class T>
T loadExtension(VkInstance instance, const char* fnName, bool isOptional = false)
{
    auto func = (T) vkGetInstanceProcAddr(instance, fnName);
    if (func == nullptr && !isOptional)
    {
        throw std::runtime_error(std::string("Error loading extension: ") + fnName);
    }
    else if (func == nullptr)
    {
        bg2e_log_warning << "\t WARN: " << fnName << " extension function not loaded" << bg2e_log_end;
    }
    if (base::Log::isDebug())
    {
        bg2e_log_debug << "\t" << fnName << bg2e_log_end;
    }
    return func;
}

void loadInstanceExtensions(VkInstance instance, bool offscreen)
{
    bg2e_log_debug << "Loading extension functions:" << bg2e_log_end;
    cmdBeginRendering = loadExtension<PFN_vkCmdBeginRenderingKHR>(instance, "vkCmdBeginRenderingKHR");
    cmdEndRendering = loadExtension<PFN_vkCmdEndRenderingKHR>(instance, "vkCmdEndRenderingKHR");
    acquireNextImage = loadExtension<PFN_vkAcquireNextImageKHR>(instance, "vkAcquireNextImageKHR");
    queuePresent = loadExtension<PFN_vkQueuePresentKHR>(instance, "vkQueuePresentKHR");
    destroySwapchain = loadExtension<PFN_vkDestroySwapchainKHR>(instance, "vkDestroySwapchainKHR");
    destroySurface = loadExtension<PFN_vkDestroySurfaceKHR>(instance, "vkDestroySurfaceKHR", offscreen);
    queueSubmit2 = loadExtension<PFN_vkQueueSubmit2KHR>(instance, "vkQueueSubmit2KHR");
    cmdPipelineBarrier2 = loadExtension<PFN_vkCmdPipelineBarrier2KHR>(instance, "vkCmdPipelineBarrier2KHR");
    cmdBlitImage2 = loadExtension<PFN_vkCmdBlitImage2KHR>(instance, "vkCmdBlitImage2KHR");

    if (base::Log::isDebug())
    {
        setDebugUtilsObjectName_OPT = loadExtension<PFN_vkSetDebugUtilsObjectNameEXT>(instance, "vkSetDebugUtilsObjectNameEXT", true);
    }
}

void loadDeviceExtensions(const PhysicalDevice& physDev, VkDevice device, bool offscreen)
{
    bg2e_log_debug << "Loading device extension functions:" << bg2e_log_end;

    if (device == VK_NULL_HANDLE) {
        bg2e_log_error << "Invalid VkDevice while loading device extension functions" << bg2e_log_end;
        return;
    }

    auto loadDeviceExtension = [device](const char* name) -> PFN_vkVoidFunction {
        PFN_vkVoidFunction fn = vkGetDeviceProcAddr(device, name);
        if (!fn) {
            bg2e_log_warning << "Device extension function not available: " << name << bg2e_log_end;
        }
        else {
            bg2e_log_debug << "Loaded device extension function: " << name << bg2e_log_end;
        }
        return fn;
    };

    if (physDev.properties()->rayTracingSupported()) {
        // Acceleration structure
        cmdBuildAccelerationStructures =
            reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(
                loadDeviceExtension("vkCmdBuildAccelerationStructuresKHR")
            );

        cmdCopyAccelerationStructure =
            reinterpret_cast<PFN_vkCmdCopyAccelerationStructureKHR>(
                loadDeviceExtension("vkCmdCopyAccelerationStructureKHR")
            );

        cmdCopyAccelerationStructureToMemory =
            reinterpret_cast<PFN_vkCmdCopyAccelerationStructureToMemoryKHR>(
                loadDeviceExtension("vkCmdCopyAccelerationStructureToMemoryKHR")
            );

        cmdCopyMemoryToAccelerationStructure =
            reinterpret_cast<PFN_vkCmdCopyMemoryToAccelerationStructureKHR>(
                loadDeviceExtension("vkCmdCopyMemoryToAccelerationStructureKHR")
            );

        cmdWriteAccelerationStructuresProperties =
            reinterpret_cast<PFN_vkCmdWriteAccelerationStructuresPropertiesKHR>(
                loadDeviceExtension("vkCmdWriteAccelerationStructuresPropertiesKHR")
            );

        getAccelerationStructureBuildSizes = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(
                loadDeviceExtension("vkGetAccelerationStructureBuildSizesKHR")
            );

        getAccelerationStructureDeviceAddress =
            reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(
                loadDeviceExtension("vkGetAccelerationStructureDeviceAddressKHR")
            );
        createAccelerationStructure =
            reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(
                loadDeviceExtension("vkCreateAccelerationStructureKHR")
            );
        destroyAccelerationStructure =
            reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(
                loadDeviceExtension("vkDestroyAccelerationStructureKHR")
            );


        // Ray tracing pipeline
        createRayTracingPipelines =
            reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(
                loadDeviceExtension("vkCreateRayTracingPipelinesKHR")
            );

        cmdTraceRays =
            reinterpret_cast<PFN_vkCmdTraceRaysKHR>(
                loadDeviceExtension("vkCmdTraceRaysKHR")
            );

        getRayTracingCaptureReplayShaderGroupHandles =
            reinterpret_cast<PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR>(
                loadDeviceExtension("vkGetRayTracingCaptureReplayShaderGroupHandlesKHR")
            );

        // Deferred host operations
        createDeferredOperation =
            reinterpret_cast<PFN_vkCreateDeferredOperationKHR>(
                loadDeviceExtension("vkCreateDeferredOperationKHR")
            );

        destroyDeferredOperation =
            reinterpret_cast<PFN_vkDestroyDeferredOperationKHR>(
                loadDeviceExtension("vkDestroyDeferredOperationKHR")
            );

        getDeferredOperationResult =
            reinterpret_cast<PFN_vkGetDeferredOperationResultKHR>(
                loadDeviceExtension("vkGetDeferredOperationResultKHR")
            );

        deferredOperationJoin =
            reinterpret_cast<PFN_vkDeferredOperationJoinKHR>(
                loadDeviceExtension("vkDeferredOperationJoinKHR")
            );
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
PFN_vkCreateAccelerationStructureKHR                   createAccelerationStructure;
PFN_vkDestroyAccelerationStructureKHR                  destroyAccelerationStructure;
PFN_vkGetAccelerationStructureDeviceAddressKHR         getAccelerationStructureDeviceAddress;

// VK_KHR_ray_tracing_pipeline
PFN_vkCreateRayTracingPipelinesKHR                     createRayTracingPipelines;
PFN_vkCmdTraceRaysKHR                                  cmdTraceRays;
PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR  getRayTracingCaptureReplayShaderGroupHandles;

// VK_KHR_deferred_host_operations
PFN_vkCreateDeferredOperationKHR                       createDeferredOperation;
PFN_vkDestroyDeferredOperationKHR                      destroyDeferredOperation;
PFN_vkGetDeferredOperationResultKHR                    getDeferredOperationResult;
PFN_vkDeferredOperationJoinKHR                         deferredOperationJoin;

// VK_EXT_DEBUG_UTILS_EXTENSION_NAME
PFN_vkSetDebugUtilsObjectNameEXT                       setDebugUtilsObjectName_OPT;

}
}
}

