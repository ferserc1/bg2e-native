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

#pragma once

#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/gpu/Common.hpp>

#ifdef BG2E_IS_MAC

#include "Metal/Metal.hpp"
#include "Foundation/Foundation.hpp"
#include "QuartzCore/QuartzCore.hpp"

namespace bg2e::gpu::metal {
    using DeviceHandle       = MTL::Device*;
    using CommandQueueHandle = MTL::CommandQueue*;
    using MetalLayerHandle   = CA::MetalLayer*;

#if BG2E_IS_MAC
    MTL::PixelFormat toMetalPixelFormat(gpu::PixelFormat format);
    gpu::PixelFormat fromMetalPixelFormat(MTL::PixelFormat format);
#endif
}

#else

namespace bg2e::gpu::metal {
    struct DeviceOpaque;
    using DeviceHandle = DeviceOpaque*;

    struct CommandQueueOpaque;
    using CommandQueueHandle = CommandQueueOpaque*;

    struct MetalLayerOpaque;
    using MetalLayerHandle = MetalLayerOpaque*;
}

#endif