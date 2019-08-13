
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

	void BufferUtils::CopyBufferToImage(vk::Instance* instance, VkCommandPool pool, vk::Buffer* srcBuffer, VkImage image, uint32_t width, uint32_t height) {
		SingleTimeCommandBuffer stcb(instance, pool);
		stcb.execute([&](vk::CommandBuffer* cb) {
			VkBufferImageCopy region = {};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;

			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;

			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = {
				width,
				height,
				1
			};

			// TODO: Refactor. Add copyBufferToImage to vk::CommandBuffer
			vkCmdCopyBufferToImage(
				cb->commandBuffer(),
				srcBuffer->buffer(),
				image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&region);
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
