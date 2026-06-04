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

#include <bg2e/gpu/metal/common.hpp>

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

#include <bg2e/gpu/metal/common.hpp>

#if BG2E_IS_MAC

#include <Metal/Metal.hpp>

namespace bg2e::gpu::metal {

MTL::PixelFormat toMetalPixelFormat(gpu::PixelFormat format)
{
    switch (format) {
        case gpu::PixelFormat::R8G8B8A8_UNORM:  return MTL::PixelFormatRGBA8Unorm;
        case gpu::PixelFormat::R8G8B8A8_SRGB:   return MTL::PixelFormatRGBA8Unorm_sRGB;
        case gpu::PixelFormat::B8G8R8A8_UNORM:  return MTL::PixelFormatBGRA8Unorm;
        case gpu::PixelFormat::B8G8R8A8_SRGB:   return MTL::PixelFormatBGRA8Unorm_sRGB;
        case gpu::PixelFormat::R16G16B16A16_SFLOAT: return MTL::PixelFormatRGBA16Float;
        case gpu::PixelFormat::R32G32B32A32_SFLOAT: return MTL::PixelFormatRGBA32Float;
        case gpu::PixelFormat::D16_UNORM:       return MTL::PixelFormatDepth16Unorm;
        case gpu::PixelFormat::D32_SFLOAT:      return MTL::PixelFormatDepth32Float;
        case gpu::PixelFormat::D24_UNORM_S8_UINT: return MTL::PixelFormatDepth24Unorm_Stencil8;
        case gpu::PixelFormat::D32_SFLOAT_S8_UINT: return MTL::PixelFormatDepth32Float_Stencil8;
        case gpu::PixelFormat::Undefined:       return MTL::PixelFormatInvalid;
        default:                                return MTL::PixelFormatInvalid;
    }
}

gpu::PixelFormat fromMetalPixelFormat(MTL::PixelFormat format)
{
    switch (format) {
        case MTL::PixelFormatRGBA8Unorm:         return gpu::PixelFormat::R8G8B8A8_UNORM;
        case MTL::PixelFormatRGBA8Unorm_sRGB:    return gpu::PixelFormat::R8G8B8A8_SRGB;
        case MTL::PixelFormatBGRA8Unorm:         return gpu::PixelFormat::B8G8R8A8_UNORM;
        case MTL::PixelFormatBGRA8Unorm_sRGB:    return gpu::PixelFormat::B8G8R8A8_SRGB;
        case MTL::PixelFormatRGBA16Float:        return gpu::PixelFormat::R16G16B16A16_SFLOAT;
        case MTL::PixelFormatRGBA32Float:        return gpu::PixelFormat::R32G32B32A32_SFLOAT;
        case MTL::PixelFormatDepth16Unorm:       return gpu::PixelFormat::D16_UNORM;
        case MTL::PixelFormatDepth32Float:       return gpu::PixelFormat::D32_SFLOAT;
        case MTL::PixelFormatDepth24Unorm_Stencil8: return gpu::PixelFormat::D24_UNORM_S8_UINT;
        case MTL::PixelFormatDepth32Float_Stencil8: return gpu::PixelFormat::D32_SFLOAT_S8_UINT;
        case MTL::PixelFormatInvalid:            return gpu::PixelFormat::Undefined;
        default:                                 return gpu::PixelFormat::Undefined;
    }
}

}

#endif