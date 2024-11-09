#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/PhysicalDevice.hpp>
#include <bg2e/render/vulkan/Surface.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

class BG2E_API Device {
public:

	void create(const Instance& instance, const PhysicalDevice& physicalDevice, const Surface& surface);
	
	void cleanup();

	inline VkDevice handle() const { return _device; }
	inline bool isValid() const { return _device != VK_NULL_HANDLE; }

    inline VkQueue graphicsQueue() const { return _graphicsQueue; }
    inline VkQueue presentQueue() const { return _presentQueue; }
    inline uint32_t graphicsFamily() const { return _graphicsFamily; }
    inline uint32_t presentFamily() const { return _presentFamily; }
    
    void waitIdle() const;

protected:
	VkDevice _device = VK_NULL_HANDLE;
    VkQueue _graphicsQueue = VK_NULL_HANDLE;
    VkQueue _presentQueue = VK_NULL_HANDLE;
    uint32_t _graphicsFamily = 0;
    uint32_t _presentFamily = 0;
};

}
}
}
