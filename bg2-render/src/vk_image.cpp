
#include <bg2render/vk_image.hpp>
#include <bg2render/single_time_command_buffer.hpp>

#include <stdexcept>

namespace bg2render {
    namespace vk {

		Image::Image(Instance* inst)
			:_instance(inst)
		{

		}

		Image::~Image() {
			if (_image != VK_NULL_HANDLE) {
				vkDestroyImage(_instance->renderDevice()->device(), _image, nullptr);
			}
		}

		void Image::create(VkFormat format, const bg2math::int2& extent, VkImageUsageFlags usage) {
			_createInfo.format = format;
			_createInfo.imageType = VK_IMAGE_TYPE_2D;
			_createInfo.extent.width = extent.x();
			_createInfo.extent.height = extent.y();
			_createInfo.extent.depth = 1;
			_createInfo.usage = usage;

			if (vkCreateImage(_instance->renderDevice()->device(), &_createInfo, nullptr, &_image) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create image");
			}

			vkGetImageMemoryRequirements(_instance->renderDevice()->device(), _image, &_memoryRequirements);
		}

		void Image::create(VkFormat format, const bg2math::int3& extent, VkImageUsageFlags usage) {
			_createInfo.format = format;
			_createInfo.imageType = VK_IMAGE_TYPE_3D;
			_createInfo.extent.width = extent.x();
			_createInfo.extent.height = extent.y();
			_createInfo.extent.depth = extent.z();
			_createInfo.usage = usage;

			if (vkCreateImage(_instance->renderDevice()->device(), &_createInfo, nullptr, &_image) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create image");
			}

			vkGetImageMemoryRequirements(_instance->renderDevice()->device(), _image, &_memoryRequirements);
		}
        
		void Image::layoutTransition(VkCommandPool commandPool, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags shaderStages) {
			bg2render::SingleTimeCommandBuffer stcb(_instance, commandPool);

			stcb.execute([&](CommandBuffer* cb) {
				VkImageMemoryBarrier barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.oldLayout = oldLayout;
				barrier.newLayout = newLayout;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.image = _image;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = 0;

				VkPipelineStageFlags sourceStage;
				VkPipelineStageFlags destinationStage;

				// Setup aspect mask
				if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

					// If have stencil component
					if (_createInfo.format == VK_FORMAT_D32_SFLOAT_S8_UINT || _createInfo.format == VK_FORMAT_D24_UNORM_S8_UINT) {
						barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				else {
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}

				if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
					barrier.srcAccessMask = 0;
					barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

					sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				}
				else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
					barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

					sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
					destinationStage = shaderStages;
				}
				else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
					barrier.srcAccessMask = 0;
					barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

					sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
				}
				else {
					throw std::invalid_argument("Unsupported image layout transition");
				}

				cb->pipelineBarrier(
					sourceStage, destinationStage,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrier);
			});
		}
    }
}