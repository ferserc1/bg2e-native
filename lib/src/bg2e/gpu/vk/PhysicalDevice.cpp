/*
 *    business grade graphic engine (bg2e engine)
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

#include <bg2e/gpu/vk/PhysicalDevice.hpp>
#include <bg2e/gpu/vk/Instance.hpp>
#include <bg2e/gpu/vk/WindowSurface.hpp>
#include <bg2e/gpu/vk/OffscreenSurface.hpp>
#include <bg2e/base/Log.hpp>

#include <algorithm>
#include <set>

namespace bg2e {
namespace gpu {
namespace vk {

// ---- QueueFamilyIndices ----

bool PhysicalDevice::QueueFamilyIndices::isComplete() const
{
    return graphics.has_value() && present.has_value();
}

bool PhysicalDevice::QueueFamilyIndices::isCompleteHeadless() const
{
    return graphics.has_value();
}

// ---- Static helpers ----

static PhysicalDevice::QueueFamilyIndices queryQueueFamiliesImpl(VkPhysicalDevice device, VkSurfaceKHR surface = VK_NULL_HANDLE)
{
    PhysicalDevice::QueueFamilyIndices result;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            result.graphics = i;
        }

        if (surface != VK_NULL_HANDLE)
        {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport)
            {
                result.present = i;
            }
        }

        if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
            !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT))
        {
            result.transfer = i;
        }

        if (surface != VK_NULL_HANDLE && result.isComplete())
        {
            break;
        }
        else if (surface == VK_NULL_HANDLE && result.isCompleteHeadless())
        {
            break;
        }

        ++i;
    }

    if (!result.transfer.has_value() && result.graphics.has_value())
    {
        result.transfer = result.graphics;
    }

    return result;
}

PhysicalDevice::QueueFamilyIndices PhysicalDevice::queryQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    return queryQueueFamiliesImpl(device, surface);
}

PhysicalDeviceProperties* PhysicalDevice::queryProperties(VkPhysicalDevice device)
{
    auto props = PhysicalDeviceProperties::create();

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);
    VkDeviceSize totalMemory = 0;
    for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; ++i)
    {
        if (memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            totalMemory += memoryProperties.memoryHeaps[i].size / (1024 * 1024);
        }
    }
    props->totalHeapMemoryMB = static_cast<uint32_t>(totalMemory);

    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        props->deviceType = PhysicalDeviceProperties::DiscreteGPU;
    }
    else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    {
        props->deviceType = PhysicalDeviceProperties::IntegratedGPU;
    }
    else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
    {
        props->deviceType = PhysicalDeviceProperties::VirtualGPU;
    }
    else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
    {
        props->deviceType = PhysicalDeviceProperties::CPU;
    }
    else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_OTHER)
    {
        props->deviceType = PhysicalDeviceProperties::Other;
    }

    props->name = deviceProperties.deviceName;
    props->vendor = deviceProperties.vendorID;
    props->id = deviceProperties.deviceID;

    // Check ray tracing capabilities
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtPipelineProps = {};
    rtPipelineProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

    VkPhysicalDeviceAccelerationStructurePropertiesKHR asProps = {};
    asProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
    asProps.pNext = &rtPipelineProps;

    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures = {};
    bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures = {};
    rtPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    rtPipelineFeatures.pNext = &bufferDeviceAddressFeatures;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR asFeatures = {};
    asFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    asFeatures.pNext = &rtPipelineFeatures;

    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures = {};
    rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
    rayQueryFeatures.pNext = &asFeatures;

    VkPhysicalDeviceFeatures2 deviceFeatures = {};
    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures.pNext = &rayQueryFeatures;

    vkGetPhysicalDeviceFeatures2(device, &deviceFeatures);

    // Check for ray tracing extensions
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> availableExtensionNames;
    for (const auto& ext : availableExtensions)
    {
        availableExtensionNames.insert(ext.extensionName);
    }

    bool hasRTExtensions =
        availableExtensionNames.count(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) &&
        availableExtensionNames.count(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) &&
        availableExtensionNames.count(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);

    props->rayTracing.available = hasRTExtensions;
    props->rayTracing.rayTracingPipeline = rtPipelineFeatures.rayTracingPipeline;
    props->rayTracing.rayQuery = rayQueryFeatures.rayQuery;
    props->rayTracing.accelerationStructure = asFeatures.accelerationStructure;
    props->rayTracing.bufferDeviceAddress = bufferDeviceAddressFeatures.bufferDeviceAddress;

    return props;
}

// ---- Extension lists ----

static const std::vector<const char*> g_requiredDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
    VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME,
    VK_KHR_MULTIVIEW_EXTENSION_NAME,
    VK_KHR_MAINTENANCE_1_EXTENSION_NAME
#ifdef __APPLE__
    ,
    "VK_KHR_portability_subset"
#endif
};

static const std::vector<const char*> g_offscreenRequiredDeviceExtensions = {
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
    VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME,
    VK_KHR_MULTIVIEW_EXTENSION_NAME,
    VK_KHR_MAINTENANCE_1_EXTENSION_NAME
#ifdef __APPLE__
    ,
    "VK_KHR_portability_subset"
#endif
};

const std::vector<const char*>& PhysicalDevice::getRequiredDeviceExtensions(bool offscreen)
{
    return offscreen ? g_offscreenRequiredDeviceExtensions : g_requiredDeviceExtensions;
}

bool PhysicalDevice::checkDeviceExtensions(VkPhysicalDevice device)
{
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    bg2e_log_debug << "Available device extensions:" << bg2e_log_end;
    std::set<std::string> requiredExtensionsSet(g_requiredDeviceExtensions.begin(), g_requiredDeviceExtensions.end());
    for (const auto& extension : availableExtensions)
    {
        bg2e_log_debug << "\t" << extension.extensionName << bg2e_log_end;
        requiredExtensionsSet.erase(extension.extensionName);
    }

    return requiredExtensionsSet.empty();
}

// ---- Suitability checks ----

bool PhysicalDevice::isSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    QueueFamilyIndices indices = queryQueueFamiliesImpl(device, surface);

    bool extensionsSupported = checkDeviceExtensions(device);

    bool swapchainAdequate = false;
    if (extensionsSupported && surface != VK_NULL_HANDLE)
    {
        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        swapchainAdequate = formatCount > 0 && presentModeCount > 0;
    }

    return indices.isComplete() && extensionsSupported && swapchainAdequate;
}

bool PhysicalDevice::isSuitableHeadless(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = queryQueueFamiliesImpl(device);

    bool extensionsSupported = checkDeviceExtensions(device);

    return indices.isCompleteHeadless() && extensionsSupported;
}

// ---- PhysicalDevice interface ----

void PhysicalDevice::choose(gpu::Instance& instance, gpu::Surface& surface)
{
    auto* vkInst = dynamic_cast<vk::Instance*>(&instance);
    VkInstance vkInstance = vkInst->vkInstanceHnd();

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);
    if (deviceCount == 0)
    {
        throw std::runtime_error("Failed to find GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data());

    std::vector<std::shared_ptr<PhysicalDeviceProperties>> suitableDevices;
    uint32_t highestScore = 0;
    std::shared_ptr<PhysicalDeviceProperties> bestDeviceProps;
    VkPhysicalDevice bestDevice = VK_NULL_HANDLE;

    if (surface.isOffscreen())
    {
        for (const auto& device : devices)
        {
            if (isSuitableHeadless(device))
            {
                suitableDevices.push_back(std::shared_ptr<PhysicalDeviceProperties>(queryProperties(device)));
                uint32_t score = suitableDevices.back()->getScore();
                if (score >= highestScore)
                {
                    highestScore = score;
                    bestDeviceProps = suitableDevices.back();
                    bestDevice = device;
                }
            }
        }
    }
    else
    {
        auto* winSurface = dynamic_cast<vk::WindowSurface*>(&surface);
        VkSurfaceKHR vkSurface = winSurface->handle();

        for (const auto& device : devices)
        {
            if (isSuitable(device, vkSurface))
            {
                suitableDevices.push_back(std::shared_ptr<PhysicalDeviceProperties>(queryProperties(device)));
                uint32_t score = suitableDevices.back()->getScore();
                if (score >= highestScore)
                {
                    highestScore = score;
                    bestDeviceProps = suitableDevices.back();
                    bestDevice = device;
                }
            }
        }
    }

    if (bestDeviceProps == nullptr)
    {
        throw std::runtime_error("Failed to find a suitable GPU");
    }

    if (surface.isOffscreen())
    {
        std::cout << "Selected GPU (headless): " << bestDeviceProps->name << std::endl;
    }
    else
    {
        std::cout << "Selected GPU: " << bestDeviceProps->name << std::endl;
    }

    _physicalDevice = bestDevice;
    _properties = bestDeviceProps;
    _surface = &surface;
}

bool PhysicalDevice::isValid() const
{
    return _physicalDevice != VK_NULL_HANDLE;
}

const std::shared_ptr<PhysicalDeviceProperties> PhysicalDevice::properties() const
{
    return _properties;
}

PhysicalDevice::QueueFamilyIndices PhysicalDevice::queueFamilyIndices() const
{
    if (!isValid())
    {
        throw std::runtime_error("PhysicalDevice::queueFamilyIndices(): No device selected.");
    }

    if (_surface == nullptr || _surface->isOffscreen())
    {
        return queryQueueFamiliesImpl(_physicalDevice);
    }

    auto* winSurface = dynamic_cast<const vk::WindowSurface*>(_surface);
    return queryQueueFamiliesImpl(_physicalDevice, winSurface->handle());
}

}
}
}
