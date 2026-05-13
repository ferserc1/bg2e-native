/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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

void ColorAttachments::build(VkExtent2D extent)
{
    _extent = extent;
    cleanup();
    
    for (auto format : _attachmentFormats)
    {
        auto image = vulkan::Image::createAllocatedImage(
            _engine,
            "ColorAttachments image buffer",
			format,
			extent,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
            1, false, 20, VK_SAMPLE_COUNT_1_BIT
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
