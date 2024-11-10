
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
    static const std::vector<const char*>& getRequiredDeviceExtensions();
    
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

		static SwapChainSupportDetails get(VkPhysicalDevice device, const Surface& surface);
		VkSurfaceFormatKHR chooseSurfaceFormat(VkFormat preferredFormat) const;
		VkPresentModeKHR choosePresentMode(VkPresentModeKHR preferredPresentMode, VkPresentModeKHR fallbackMode = VK_PRESENT_MODE_FIFO_KHR) const;
		VkExtent2D chooseExtent(const Surface& surface) const;
        uint32_t imageCount() const;
    };

    void choose(const Instance& instance, const Surface& surface);
    
    QueueFamilyIndices queueFamilyIndices() const;

	inline VkPhysicalDevice handle() const { return _device; }
	inline bool isValid() const { return _device != VK_NULL_HANDLE; }
    
    inline const Surface& surface() const
    {
        if (!_surface)
        {
            throw new std::runtime_error("No physical device choosen");
        }
        return *_surface;
    }
  
protected:
	VkPhysicalDevice _device = VK_NULL_HANDLE;
    const Surface * _surface = nullptr;

    bool isSuitable(VkPhysicalDevice device, const Surface& surface);
    bool checkDeviceExtensions(VkPhysicalDevice device);
};

}
}
}
