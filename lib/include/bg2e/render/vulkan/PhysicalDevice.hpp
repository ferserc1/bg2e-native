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

#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/Surface.hpp>

#include <SDL2/SDL.h>

#include <optional>
#include <vector>

namespace bg2e {
namespace render {
namespace vulkan {

class Instance;

struct RayTracingCapabilities {
    bool available = false;
    bool rayTracingPipeline = false;
    bool rayQuery = false;
    bool accelerationStructure = false;
    bool bufferDeviceAddress = false;

    inline bool fullSupported() const
    {
        return available &&
            rayTracingPipeline &&
            rayQuery &&
            accelerationStructure &&
            bufferDeviceAddress;
    }
};

struct PhysicalDeviceProperties {
    enum DeviceType {
        IntegratedGPU,
        DiscreteGPU,
        VirtualGPU,
        CPU,
        Other
    } deviceType = Other;

    size_t totalHeapMemoryMB = 0;
    DeviceType type = Other;
    std::string name;
    uint32_t vendor = 0;
    uint32_t id = 0;
    VkPhysicalDevice deviceHandle = VK_NULL_HANDLE;

    RayTracingCapabilities rayTracing;

    static PhysicalDeviceProperties * query(VkPhysicalDevice device);

    uint32_t getScore() const;

    bool rayTracingSupported() const;

private:
    PhysicalDeviceProperties() = default;
};

class PhysicalDevice {
public:
    static const std::vector<const char*>& getRequiredDeviceExtensions(bool offscreen);
    
    static void listSuitableDevices(
        const Instance& instance,
        const Surface& surface,
        std::vector<std::shared_ptr<PhysicalDeviceProperties>>& result
    );
    
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> present;
        
        inline bool isComplete()
        {
            return graphics.has_value() && present.has_value();
        }

        inline bool isCompleteHeadless()
        {
            return graphics.has_value();
        }
        
        static QueueFamilyIndices get(VkPhysicalDevice device, const Surface& surface);
        static QueueFamilyIndices graphicsOnly(VkPhysicalDevice device);
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
        VkSampleCountFlagBits maxMSAASamples;

		static SwapChainSupportDetails get(VkPhysicalDevice device, const Surface& surface);
		VkSurfaceFormatKHR chooseSurfaceFormat(VkFormat preferredFormat) const;
		VkPresentModeKHR choosePresentMode(VkPresentModeKHR preferredPresentMode, VkPresentModeKHR fallbackMode = VK_PRESENT_MODE_FIFO_KHR) const;
		VkExtent2D chooseExtent(const Surface& surface) const;
        uint32_t imageCount() const;
    };

    void choose(const Instance& instance, const Surface& surface);

    // Used for offscreen applications
    void choose(const Instance& instance);

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

    const std::shared_ptr<PhysicalDeviceProperties> properties() const { return _properties; }

protected:
	VkPhysicalDevice _device = VK_NULL_HANDLE;
    const Surface * _surface = nullptr;
    std::shared_ptr<PhysicalDeviceProperties> _properties;

    static bool isSuitable(VkPhysicalDevice device, const Surface& surface);
    static bool isSuitableHeadless(VkPhysicalDevice device);
    static bool checkDeviceExtensions(VkPhysicalDevice device);
};

}
}
}
