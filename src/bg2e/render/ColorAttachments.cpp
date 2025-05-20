//
//  ColorAttachments.cpp

#include <bg2e/render/ColorAttachments.hpp>

namespace bg2e::render {

ColorAttachments::ColorAttachments(Vulkan * vulkan, const std::vector<VkFormat>& formats)
    :_vulkan{ vulkan }
{
    _attachmentFormats.assign(formats.begin(), formats.end());
}

ColorAttachments::~ColorAttachments()
{
    cleanup();
}

void ColorAttachments::build(VkExtent2D extent)
{
    _extent = extent;
    cleanup();
    
    for (auto format : _attachmentFormats)
    {
        auto image = vulkan::Image::createAllocatedImage(
            _vulkan,
			format,
			extent,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT
        );
        _images.push_back(std::shared_ptr<vulkan::Image>(image));
        _targetImages.push_back(image);
    }
}

std::shared_ptr<vulkan::Image> ColorAttachments::imageWithIndex(uint32_t index) const
{
    return _images[index];
}

void ColorAttachments::cleanup()
{
    _images.clear();
    _targetImages.clear();
}

}
