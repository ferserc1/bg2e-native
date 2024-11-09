
#pragma once

#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/Surface.hpp>

#include <SDL2/SDL.h>

#include <optional>
#include <vector>

namespace bg2e {
namespace render {
namespace vulkan {

class Instance;

class PhysicalDevice {
public:
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> present;
        
        inline bool isComplete()
        {
            return graphics.has_value() && present.has_value();
        }
        
        static QueueFamilyIndices get(VkPhysicalDevice device, const Surface& surface);
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;

		static SwapChainSupportDetails getSwapChainSupport(VkPhysicalDevice device, const Surface& surface);
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(VkFormat preferredFormat);
		VkPresentModeKHR chooseSwapPresentMode(VkPresentModeKHR preferredPresentMode, VkPresentModeKHR fallbackMode = VK_PRESENT_MODE_FIFO_KHR);
		VkExtent2D chooseSwapExtent(const Surface& surface);
    };

    void choose(const Instance& instance, const Surface& surface);

	inline VkPhysicalDevice handle() const { return _device; }
	inline bool isValid() { return _device != VK_NULL_HANDLE; }
  
protected:
	VkPhysicalDevice _device = VK_NULL_HANDLE;

    bool isSuitable(VkPhysicalDevice device, const Surface& surface);
    bool checkDeviceExtensions(VkPhysicalDevice device);
};

}
}
}
