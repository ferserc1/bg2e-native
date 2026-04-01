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

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/Image.hpp>

#include <vector>

namespace bg2e {
namespace render {
namespace vulkan {
namespace macros {

extern BG2E_API void cmdSetDefaultViewportAndScissor(VkCommandBuffer cmd, VkExtent2D extent);

extern BG2E_API void cmdClearImageAndBeginRendering(
    VkCommandBuffer cmd,
    const Image * colorImage,
    VkClearColorValue clearValue,
    VkImageLayout colorImageInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    const Image * depthImage = nullptr,
    float depthValue = 1.0f
);


extern BG2E_API void cmdClearImagesAndBeginRendering(
    VkCommandBuffer cmd,
    const std::vector<const Image *>& colorImages,
    VkClearColorValue clearValue,
    VkImageLayout colorImageInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    const Image * depthImage = nullptr,
    float depthValue = 1.0f
);


extern BG2E_API void cmdClearImageAndSetLayout(
    VkCommandBuffer cmd,
    const Image* colorImage,
    VkClearColorValue clearValue,
    VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
);

}
}
}
}
