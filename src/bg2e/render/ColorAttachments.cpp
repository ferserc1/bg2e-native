//
//  ColorAttachments.cpp

#include <bg2e/render/ColorAttachments.hpp>

namespace bg2e::render {

ColorAttachments::ColorAttachments(Engine * engine, const std::vector<VkFormat>& formats)
    :_engine{ engine }
{
    _attachmentFormats.assign(formats.begin(), formats.end());
}

ColorAttachments::~ColorAttachments()
{
    cleanup();
}

void ColorAttachments::build(VkExtent2D extent, VkSampleCountFlagBits samples)
{
    _extent = extent;
    _samples = samples;
    cleanup();
    
    for (auto format : _attachmentFormats)
    {
        auto image = vulkan::Image::createAllocatedImage(
            _engine,
			format,
			extent,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
            1, false, 0,
            _samples
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
