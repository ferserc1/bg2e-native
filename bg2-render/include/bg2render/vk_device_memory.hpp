#ifndef _bg2render_vk_device_memory_hpp_
#define _bg2render_vk_device_memory_hpp_

#include <vulkan/vulkan.h>
#include <bg2render/vk_instance.hpp>

namespace bg2render {
    namespace vk {

		class DeviceMemory {
		public:
			DeviceMemory(Instance*);
			virtual ~DeviceMemory();

			void allocate(const VkMemoryRequirements & req, VkMemoryPropertyFlags properties);
			void map(VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData);
			void unmap();

			inline VkDeviceMemory deviceMemory() const { return _deviceMemory; }

		protected:
			Instance* _instance;
			VkDeviceMemory _deviceMemory = VK_NULL_HANDLE;


		};
    }
}

#endif
