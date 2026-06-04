/*
 *    business grade graphic engine (bg2e engine)
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

#include <bg2e/gpu/vk/common.hpp>

namespace bg2e::gpu::vk {

VkFormat toVkFormat(gpu::PixelFormat format)
{
    switch (format) {
        case gpu::PixelFormat::R8G8B8A8_UNORM:  return VK_FORMAT_R8G8B8A8_UNORM;
        case gpu::PixelFormat::R8G8B8A8_SRGB:   return VK_FORMAT_R8G8B8A8_SRGB;
        case gpu::PixelFormat::B8G8R8A8_UNORM:  return VK_FORMAT_B8G8R8A8_UNORM;
        case gpu::PixelFormat::B8G8R8A8_SRGB:   return VK_FORMAT_B8G8R8A8_SRGB;
        case gpu::PixelFormat::R16G16B16A16_SFLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
        case gpu::PixelFormat::R32G32B32A32_SFLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case gpu::PixelFormat::D16_UNORM:       return VK_FORMAT_D16_UNORM;
        case gpu::PixelFormat::D32_SFLOAT:      return VK_FORMAT_D32_SFLOAT;
        case gpu::PixelFormat::D24_UNORM_S8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT;
        case gpu::PixelFormat::D32_SFLOAT_S8_UINT: return VK_FORMAT_D32_SFLOAT_S8_UINT;
        case gpu::PixelFormat::Undefined:       return VK_FORMAT_UNDEFINED;
        default:                                return VK_FORMAT_UNDEFINED;
    }
}

gpu::PixelFormat fromVkFormat(VkFormat format)
{
    switch (format) {
        case VK_FORMAT_R8G8B8A8_UNORM:  return gpu::PixelFormat::R8G8B8A8_UNORM;
        case VK_FORMAT_R8G8B8A8_SRGB:   return gpu::PixelFormat::R8G8B8A8_SRGB;
        case VK_FORMAT_B8G8R8A8_UNORM:  return gpu::PixelFormat::B8G8R8A8_UNORM;
        case VK_FORMAT_B8G8R8A8_SRGB:   return gpu::PixelFormat::B8G8R8A8_SRGB;
        case VK_FORMAT_R16G16B16A16_SFLOAT: return gpu::PixelFormat::R16G16B16A16_SFLOAT;
        case VK_FORMAT_R32G32B32A32_SFLOAT: return gpu::PixelFormat::R32G32B32A32_SFLOAT;
        case VK_FORMAT_D16_UNORM:       return gpu::PixelFormat::D16_UNORM;
        case VK_FORMAT_D32_SFLOAT:      return gpu::PixelFormat::D32_SFLOAT;
        case VK_FORMAT_D24_UNORM_S8_UINT: return gpu::PixelFormat::D24_UNORM_S8_UINT;
        case VK_FORMAT_D32_SFLOAT_S8_UINT: return gpu::PixelFormat::D32_SFLOAT_S8_UINT;
        case VK_FORMAT_UNDEFINED:       return gpu::PixelFormat::Undefined;
        default:                        return gpu::PixelFormat::Undefined;
    }
}

}