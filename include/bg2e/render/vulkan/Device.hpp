#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/PhysicalDevice.hpp>

namespace bg2e {
namespace render {
namespace vulkan {

class BG2E_API Device {
public:

	void create(const PhysicalDevice& physicalDevice);
	
	void cleanup();

	inline VkDevice handle() const { return _device; }
	inline bool isValid() { return _device != VK_NULL_HANDLE; }

protected:
	VkDevice _device = VK_NULL_HANDLE;
};

}
}
}
