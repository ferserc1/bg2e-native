
#include <bg2render/buffer_utils.hpp>
#include <bg2render/vk_command_buffer.hpp>
#include <bg2render/single_time_command_buffer.hpp>

namespace bg2render {

	void BufferUtils::CopyBuffer(vk::Instance* instance, VkCommandPool pool, vk::Buffer* srcBuffer, vk::Buffer* dstBuffer) {
		SingleTimeCommandBuffer stcb(instance, pool);
		stcb.execute([&](vk::CommandBuffer* cb) {
			cb->copyBuffer(srcBuffer, dstBuffer);
		});
	}

	void BufferUtils::CreateBufferMemory(vk::Instance * instance, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, vk::Buffer*& buffer, vk::DeviceMemory*& memory) {
		buffer = new vk::Buffer(instance);
		buffer->create(
			size,
			usage,
			VK_SHARING_MODE_EXCLUSIVE
		);
		memory = new vk::DeviceMemory(instance);
		memory->allocate(
			buffer->memoryRequirements(),
			properties
		);
		vkBindBufferMemory(instance->renderDevice()->device(), buffer->buffer(), memory->deviceMemory(), 0);
	}

	void BufferUtils::CreateBufferMemory(vk::Instance* instance, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, std::shared_ptr<vk::Buffer>& buffer, std::shared_ptr<vk::DeviceMemory>& memory) {
		vk::Buffer* resultBuffer;
		vk::DeviceMemory* resultMemory;
		CreateBufferMemory(instance, size, usage, properties, resultBuffer, resultMemory);
		buffer = std::shared_ptr<vk::Buffer>(resultBuffer);
		memory = std::shared_ptr<vk::DeviceMemory>(resultMemory);
	}

	void BufferUtils::CreateBufferMemory(vk::Instance* instance, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, std::unique_ptr<vk::Buffer>& buffer, std::unique_ptr<vk::DeviceMemory>& memory) {
		vk::Buffer* resultBuffer;
		vk::DeviceMemory* resultMemory;
		CreateBufferMemory(instance, size, usage, properties, resultBuffer, resultMemory);
		buffer = std::unique_ptr<vk::Buffer>(resultBuffer);
		memory = std::unique_ptr<vk::DeviceMemory>(resultMemory);
	}
}
