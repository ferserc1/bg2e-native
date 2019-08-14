
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

	void BufferUtils::CopyBufferToImage(vk::Instance* instance, VkCommandPool pool, vk::Buffer* srcBuffer, vk::Image* image, uint32_t width, uint32_t height) {
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
				image->image(),
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

	void BufferUtils::CreateImageMemory(vk::Instance* instance, VkCommandPool pool, bg2base::image* imageData, vk::Image*& image, vk::DeviceMemory*& memory, vk::ImageView*& imageView, VkPipelineStageFlags shaderStages) {
		auto stagingBuffer = std::make_unique<bg2render::vk::Buffer>(instance);
		auto stagingBufferMemory = std::make_unique<bg2render::vk::DeviceMemory>(instance);
		VkDeviceSize size = imageData->size().x() * imageData->size().y() * imageData->bytesPerPixel();
		CreateBufferMemory(
			instance, size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory
		);

		void* data;
		stagingBufferMemory->map(0, size, 0, &data);
		memcpy(data, imageData->data(), static_cast<size_t>(size));
		stagingBufferMemory->unmap();

		image = new vk::Image(instance);
		image->create(
			VK_FORMAT_R8G8B8A8_UNORM,
			imageData->size(),
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
		);

		memory = new vk::DeviceMemory(instance);
		memory->allocate(image->memoryRequirements(), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		vkBindImageMemory(instance->renderDevice()->device(), image->image(), memory->deviceMemory(), 0);

		image->layoutTransition(
			pool,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		CopyBufferToImage(instance, pool, stagingBuffer.get(), image, imageData->size().x(), imageData->size().y());

		image->layoutTransition(pool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, shaderStages);

		imageView = new vk::ImageView(instance);
		imageView->create(image, image->format());
	}

	void BufferUtils::CreateImageMemory(vk::Instance* instance, VkCommandPool pool, bg2base::image* imageData, std::shared_ptr<vk::Image>& image, std::shared_ptr<vk::DeviceMemory>& memory, std::shared_ptr<vk::ImageView>& imageView, VkPipelineStageFlags shaderStages) {
		vk::Image* resultImage;
		vk::DeviceMemory* resultDeviceMemory;
		vk::ImageView* resultImageView;
		CreateImageMemory(instance, pool, imageData, resultImage, resultDeviceMemory, resultImageView, shaderStages);
		image = std::shared_ptr<vk::Image>(resultImage);
		memory = std::shared_ptr<vk::DeviceMemory>(resultDeviceMemory);
		imageView = std::shared_ptr<vk::ImageView>(resultImageView);
	}

	void BufferUtils::CreateImageMemory(vk::Instance* instance, VkCommandPool pool, bg2base::image* imageData, std::unique_ptr<vk::Image>& image, std::unique_ptr<vk::DeviceMemory>& memory, std::unique_ptr<vk::ImageView>& imageView, VkPipelineStageFlags shaderStages) {
		vk::Image* resultImage;
		vk::DeviceMemory* resultDeviceMemory;
		vk::ImageView* resultImageView;
		CreateImageMemory(instance, pool, imageData, resultImage, resultDeviceMemory, resultImageView, shaderStages);
		image = std::unique_ptr<vk::Image>(resultImage);
		memory = std::unique_ptr<vk::DeviceMemory>(resultDeviceMemory);
		imageView = std::unique_ptr<vk::ImageView>(resultImageView);
	}
}
