
#include <bg2render/vk_image_view.hpp>

#include <stdexcept>

namespace bg2render {
	namespace vk {

		ImageView::ImageView(Instance* inst)
			:_instance(inst)
		{

		}

		ImageView::~ImageView() {
			if (_imageView != VK_NULL_HANDLE) {
				vkDestroyImageView(_instance->renderDevice()->device(), _imageView, nullptr);
			}
		}

		void ImageView::create(VkImage image, VkFormat format) {
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;
			if (vkCreateImageView(_instance->renderDevice()->device(), &viewInfo, nullptr, &_imageView) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create image view");
			}
		}
	}
}