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

#include <bg2e/render/vulkan/PhysicalDevice.hpp>
#include <bg2e/base/Log.hpp>

#include <algorithm>
#include <set>

namespace bg2e {
namespace render {
namespace vulkan {

PhysicalDeviceProperties * PhysicalDeviceProperties::query(VkPhysicalDevice device)
{
    auto props = new PhysicalDeviceProperties();

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);
    VkDeviceSize totalMemory = 0;
    for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; ++i)
    {
        if (memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT){
            totalMemory += memoryProperties.memoryHeaps[i].size / (1024 * 1024);
        }
    }
    props->totalHeapMemoryMB = static_cast<uint32_t>(totalMemory);

    // Discrete GPUs have a significant performance advantage
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

    props->deviceHandle = device;

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
    for (const auto& ext : availableExtensions) {
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

uint32_t PhysicalDeviceProperties::getScore() const
{
    size_t score = totalHeapMemoryMB;

    if (deviceType == DiscreteGPU)
    {
        score *= 100;
    }
    else if (deviceType == IntegratedGPU)
    {
        score *= 10;
    }
    else if (deviceType == VirtualGPU)
    {
        score *= 5;
    }
    else if (deviceType == CPU)
    {
        score += 1;
    }

    if (rayTracingSupported())
    {
        score *= 100;
    }

    return static_cast<uint32_t>(score);
}

bool PhysicalDeviceProperties::rayTracingSupported() const
{
    return rayTracing.fullSupported();
}

void PhysicalDevice::listSuitableDevices(
    VkInstance instance,
    const Surface& surface,
    std::vector<std::shared_ptr<PhysicalDeviceProperties>>& result
) {
    uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0)
	{
		throw std::runtime_error("Failed to find GPUs with Vulkan support");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    
    for (const auto& device : devices)
    {
        if (isSuitable(device, surface))
        {
            result.push_back(std::shared_ptr<PhysicalDeviceProperties>(PhysicalDeviceProperties::query(device)));
        }
    }
}

static PhysicalDevice::QueueFamilyIndices queryQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface = VK_NULL_HANDLE, bool checkPresent = false)
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

        if (checkPresent && surface != VK_NULL_HANDLE)
        {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport)
            {
                result.present = i;
            }
        }

        if (result.isComplete())
        {
            break;
        }

        ++i;
    }

    return result;
}

PhysicalDevice::QueueFamilyIndices PhysicalDevice::QueueFamilyIndices::get(VkPhysicalDevice device, const Surface& surface)
{
    return queryQueueFamilies(device, surface.handle(), true);
}

PhysicalDevice::QueueFamilyIndices PhysicalDevice::QueueFamilyIndices::graphicsOnly(VkPhysicalDevice device)
{
    return queryQueueFamilies(device, VK_NULL_HANDLE, false);
}

PhysicalDevice::SwapChainSupportDetails PhysicalDevice::SwapChainSupportDetails::get(VkPhysicalDevice device, const Surface & surface)
{
    SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface.handle(), &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface.handle(), &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface.handle(), &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface.handle(), &presentModeCount, nullptr);
	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface.handle(), &presentModeCount, details.presentModes.data());
	}
 
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    VkSampleCountFlags msaaSamples =
        deviceProperties.limits.framebufferColorSampleCounts &
        deviceProperties.limits.framebufferDepthSampleCounts;
    
    if (msaaSamples & VK_SAMPLE_COUNT_64_BIT)
    {
        details.maxMSAASamples = VK_SAMPLE_COUNT_64_BIT;
    }
	else if (msaaSamples & VK_SAMPLE_COUNT_32_BIT)
	{
		details.maxMSAASamples = VK_SAMPLE_COUNT_32_BIT;
	}
	else if (msaaSamples & VK_SAMPLE_COUNT_16_BIT)
	{
		details.maxMSAASamples = VK_SAMPLE_COUNT_16_BIT;
	}
	else if (msaaSamples & VK_SAMPLE_COUNT_8_BIT)
	{
		details.maxMSAASamples = VK_SAMPLE_COUNT_8_BIT;
	}
	else if (msaaSamples & VK_SAMPLE_COUNT_4_BIT)
	{
		details.maxMSAASamples = VK_SAMPLE_COUNT_4_BIT;
	}
	else if (msaaSamples & VK_SAMPLE_COUNT_2_BIT)
	{
		details.maxMSAASamples = VK_SAMPLE_COUNT_2_BIT;
	}
    else
    {
        details.maxMSAASamples = VK_SAMPLE_COUNT_1_BIT; // No MSAA support
    }

    return details;
}

VkSurfaceFormatKHR PhysicalDevice::SwapChainSupportDetails::chooseSurfaceFormat(VkFormat /* preferredFormat */) const
{
    for (const auto& availableFormat : formats)
    {
        if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return formats[0];
}

VkPresentModeKHR PhysicalDevice::SwapChainSupportDetails::choosePresentMode(
    VkPresentModeKHR /* preferredPresentMode */,
    VkPresentModeKHR fallbackMode
) const {
    for (const auto& availablePresentModes : presentModes)
    {
        if (availablePresentModes == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentModes;
        }
    }
    return fallbackMode;
}

VkExtent2D PhysicalDevice::SwapChainSupportDetails::chooseExtent(const Surface& surface) const
{
    VkExtent2D actualExtent = surface.getExtent();

    actualExtent.width = std::clamp(
        actualExtent.width,
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width
    );
	actualExtent.height = std::clamp(
		actualExtent.height,
		capabilities.minImageExtent.height,
		capabilities.maxImageExtent.height
	);

    return actualExtent;
}

uint32_t PhysicalDevice::SwapChainSupportDetails::imageCount() const
{
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }
    return imageCount;
}


void PhysicalDevice::choose(VkInstance instance, const Surface & surface)
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0)
	{
		throw std::runtime_error("Failed to find GPUs with Vulkan support");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());


    std::vector<std::shared_ptr<PhysicalDeviceProperties>> suitableDevices;
    uint32_t highestScore = 0;
    std::shared_ptr<PhysicalDeviceProperties> bestDeviceProps;
    for (const auto& device : devices)
    {
        if (isSuitable(device, surface))
        {
            suitableDevices.push_back(std::shared_ptr<PhysicalDeviceProperties>(PhysicalDeviceProperties::query(device)));
            uint32_t score = suitableDevices.back()->getScore();
            if (score >= highestScore)
            {
                highestScore = score;
                bestDeviceProps = suitableDevices.back();
            }
        }
    }

    if (bestDeviceProps == nullptr)
    {
		throw std::runtime_error("Failed to find a suitable GPU");
    }

    std::cout << "Selected GPU: " << bestDeviceProps->name << std::endl;

    _device = bestDeviceProps->deviceHandle;
    _surface = &surface;
    _properties = bestDeviceProps;
}

void PhysicalDevice::choose(VkInstance instance)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0)
    {
        throw std::runtime_error("Failed to find GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    std::vector<std::shared_ptr<PhysicalDeviceProperties>> suitableDevices;
    uint32_t highestScore = 0;
    std::shared_ptr<PhysicalDeviceProperties> bestDeviceProps;
    for (const auto& device : devices)
    {
        if (isSuitableHeadless(device))
        {
            suitableDevices.push_back(std::shared_ptr<PhysicalDeviceProperties>(PhysicalDeviceProperties::query(device)));
            uint32_t score = suitableDevices.back()->getScore();
            if (score >= highestScore)
            {
                highestScore = score;
                bestDeviceProps = suitableDevices.back();
            }
        }
    }

    if (bestDeviceProps == nullptr)
    {
        throw std::runtime_error("Failed to find a suitable GPU");
    }

    std::cout << "Selected GPU (headless): " << bestDeviceProps->name << std::endl;

    _device = bestDeviceProps->deviceHandle;
    _properties = bestDeviceProps;
}

PhysicalDevice::QueueFamilyIndices PhysicalDevice::queueFamilyIndices() const
{
    if (!isValid())
    {
        throw std::runtime_error("PhysicalDevice::queueFamilyIndices(): No device selected.");
    }

    if (_surface == nullptr)
    {
        return QueueFamilyIndices::graphicsOnly(_device);
    }
    
    return QueueFamilyIndices::get(_device, *_surface);
}

bool PhysicalDevice::isSuitable(VkPhysicalDevice device, const Surface &surface)
{
	QueueFamilyIndices indices = QueueFamilyIndices::get(device, surface);

    bool extensionsSupported = checkDeviceExtensions(device);

	bool swapchainAdequate = false;
    if (extensionsSupported)
    {
		SwapChainSupportDetails swapChainSupport = SwapChainSupportDetails::get(device, surface);
		swapchainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapchainAdequate;
}

bool PhysicalDevice::isSuitableHeadless(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = QueueFamilyIndices::graphicsOnly(device);

    bool extensionsSupported = checkDeviceExtensions(device);

    return indices.isCompleteHeadless() && extensionsSupported;
}

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


}
}
}
