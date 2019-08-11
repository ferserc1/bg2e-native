#ifndef _bg2render_vk_buffer_hpp_
#define _bg2render_vk_buffer_hpp_

#include <vulkan/vulkan.h>

#include <bg2render/vk_instance.hpp>

namespace bg2render {
    namespace vk {
		
		class Buffer {
		public:
			Buffer(Instance* inst);
			virtual ~Buffer();

			inline void create(size_t size, VkBufferUsageFlags usage, VkSharingMode sharingMode) {
				create(static_cast<uint32_t>(size), usage, sharingMode);
			}
			void create(uint32_t size, VkBufferUsageFlags usage, VkSharingMode sharingMode);

			inline VkBuffer buffer() const { return _buffer; }
			inline uint32_t size() const { return _size; }
			inline const VkMemoryRequirements& memoryRequirements() const { return _memoryRequirements; }

		protected:
			Instance* _instance;
			VkBuffer _buffer = VK_NULL_HANDLE;
			uint32_t _size = 0;
			VkMemoryRequirements _memoryRequirements;
		};
    }
}

#endif
