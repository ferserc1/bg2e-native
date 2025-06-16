
#include <bg2e/render/vulkan/PhysicalDevice.hpp>
#include <bg2e/render/vulkan/Instance.hpp>
#include <bg2e/base/Log.hpp>

#include <algorithm>
#include <set>

namespace bg2e {
namespace render {
namespace vulkan {

PhysicalDevice::QueueFamilyIndices PhysicalDevice::QueueFamilyIndices::get(VkPhysicalDevice device, const Surface& surface)
{
    QueueFamilyIndices result;

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

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface.handle(), &presentSupport);
        if (presentSupport)
        {
            result.present = i;
        }

        if (result.isComplete())
        {
            break;
        }

        ++i;
    }

    return result;
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

    return details;
}

VkSurfaceFormatKHR PhysicalDevice::SwapChainSupportDetails::chooseSurfaceFormat(VkFormat preferredFormat) const
{
    for (const auto& availableFormat : formats)
    {
        if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return formats[0];
}

VkPresentModeKHR PhysicalDevice::SwapChainSupportDetails::choosePresentMode(
    VkPresentModeKHR preferredPresentMode,
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

void PhysicalDevice::choose(const Instance& instance, const Surface & surface)
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance.handle(), &deviceCount, nullptr);
	if (deviceCount == 0)
	{
		throw std::runtime_error("Failed to find GPUs with Vulkan support");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance.handle(), &deviceCount, devices.data());

    // TODO: Pick the best physical device, not only the first suitable
    for (const auto& device : devices)
    {
        if (isSuitable(device, surface))
        {
            _device = device;
            break;
        }
    }

    if (_device == VK_NULL_HANDLE)
    {
		throw std::runtime_error("Failed to find a suitable GPU");
    }
    
    _surface = &surface;
}

PhysicalDevice::QueueFamilyIndices PhysicalDevice::queueFamilyIndices() const
{
    if (!isValid())
    {
        throw std::runtime_error("PhysicalDevice::queueFamilyIndices(): No device selected.");
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

const std::vector<const char*>& PhysicalDevice::getRequiredDeviceExtensions()
{
    return g_requiredDeviceExtensions;
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
