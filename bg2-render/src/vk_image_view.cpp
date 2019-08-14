
#include <bg2render/vk_image_view.hpp>

#include <stdexcept>

namespace bg2render {
	namespace vk {

		ImageView::ImageView(Instance* inst)
			:_device(inst->renderDevice())
		{

		}

		ImageView::ImageView(Device* dev)
			: _device(dev)
		{

		}

		ImageView::~ImageView() {
			if (_imageView != VK_NULL_HANDLE) {
				vkDestroyImageView(_device->device(), _imageView, nullptr);
			}
		}

		void ImageView::create(vk::Image* image, VkFormat format, VkImageAspectFlags aspectFlags) {
			create(image->image(), format, aspectFlags);
		}

		void ImageView::create(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = format;
			viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.subresourceRange.aspectMask = aspectFlags;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;
			if (vkCreateImageView(_device->device(), &viewInfo, nullptr, &_imageView) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create image view");
			}
		}
	}
}