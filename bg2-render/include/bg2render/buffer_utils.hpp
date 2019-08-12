
#ifndef _bg2render_buffer_utils_hpp_
#define _bg2render_buffer_utils_hpp_

#include <bg2render/vk_instance.hpp>
#include <bg2render/vk_buffer.hpp>
#include <bg2render/vk_device_memory.hpp>

namespace bg2render {

    class BufferUtils {
	public:
		static void CopyBuffer(vk::Instance * instance, VkCommandPool pool, vk::Buffer * srcBuffer, vk::Buffer * dstBuffer);

		static void CreateBufferMemory(vk::Instance* instance, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, vk::Buffer *& buffer, vk::DeviceMemory *& memory);

		static void CreateBufferMemory(vk::Instance* instance, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, std::shared_ptr<vk::Buffer>& buffer, std::shared_ptr<vk::DeviceMemory>& memory);
		static void CreateBufferMemory(vk::Instance* instance, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, std::unique_ptr<vk::Buffer>& buffer, std::unique_ptr<vk::DeviceMemory>& memory);
    };

}

#endif
