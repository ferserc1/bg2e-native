
#include <bg2render/vk_device_memory.hpp>
#include <stdexcept>

namespace bg2render {
    namespace vk {
		DeviceMemory::DeviceMemory(Instance* inst)
			:_instance(inst)
		{

		}

		DeviceMemory::~DeviceMemory() {
			if (_deviceMemory != VK_NULL_HANDLE) {
				vkFreeMemory(_instance->renderDevice()->device(), _deviceMemory, nullptr);
			}
		}

		void DeviceMemory::allocate(const VkMemoryRequirements& req, VkMemoryPropertyFlags properties) {
			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = req.size;
			allocInfo.memoryTypeIndex = _instance->renderPhysicalDevice()->findMemoryType(
				req.memoryTypeBits,
				properties
			);
			if (vkAllocateMemory(_instance->renderDevice()->device(), &allocInfo, nullptr, &_deviceMemory) != VK_SUCCESS) {
				throw std::runtime_error("Failed to allocate device memory");
			}
		}

		void DeviceMemory::map(VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData) {
			vkMapMemory(_instance->renderDevice()->device(), _deviceMemory, offset, size, flags, ppData);
		}

		void DeviceMemory::unmap() {
			vkUnmapMemory(_instance->renderDevice()->device(), _deviceMemory);
		}
    }
}