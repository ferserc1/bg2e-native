/*
 *    business grade graphic engine (bg2e engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of GNU General Public License as published by
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

#pragma once

#include <bg2e/gpu/Image.hpp>
#include <bg2e/gpu/vk/common.hpp>

namespace bg2e {
namespace gpu {
namespace vk {

class Image : public gpu::Image {
public:
    void cleanup() override;

    uint32_t width() const override { return _width; }
    uint32_t height() const override { return _height; }

    inline VkImage handle() const { return _image; }
    inline VkImageView imageView() const { return _imageView; }
    inline VmaAllocation allocation() const { return _allocation; }

private:
    VkImage _image{VK_NULL_HANDLE};
    VkImageView _imageView{VK_NULL_HANDLE};
    VmaAllocation _allocation{VK_NULL_HANDLE};
    uint32_t _width = 0;
    uint32_t _height = 0;
};

}
}
}
