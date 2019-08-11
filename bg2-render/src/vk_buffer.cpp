
#include <bg2render/vk_buffer.hpp>

#include <stdexcept>

namespace bg2render {
    namespace vk {
        Buffer::Buffer(Instance* inst)
            :_instance(inst)
        {

        }

		Buffer::~Buffer() {
			if (_buffer != VK_NULL_HANDLE) {
				vkDestroyBuffer(_instance->renderDevice()->device(), _buffer, nullptr);
			}
        }

		void Buffer::create(uint32_t size, VkBufferUsageFlags usage, VkSharingMode sharingMode) {
			_size = size;
			VkBufferCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			createInfo.size = size;
			createInfo.usage = usage;
			createInfo.sharingMode = sharingMode;
			if (vkCreateBuffer(_instance->renderDevice()->device(), &createInfo, nullptr, &_buffer) != VK_NULL_HANDLE) {
				throw std::runtime_error("Failed to create buffer");
			}

			// Get memory requirements 
			vkGetBufferMemoryRequirements(_instance->renderDevice()->device(), _buffer, &_memoryRequirements);
        }

    }
}