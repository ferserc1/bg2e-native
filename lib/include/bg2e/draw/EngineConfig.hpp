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
#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/gpu/Common.hpp>

#include <string>

namespace bg2e {
namespace draw {

// Configuration for the experimental draw engine. The backend field selects
// the low-level GPU API only; the high-level execution path is selected by
// the MainLoop run overload.
struct EngineConfig {
    // Prefer the native Metal backend on macOS; use Vulkan elsewhere.
#ifdef BG2E_IS_MAC
    gpu::BackendType backend = gpu::BackendType::Metal;
#else
    gpu::BackendType backend = gpu::BackendType::Vulkan;
#endif
    bool debug = false;

    // Empty by default: the MainLoop appId is used as the fallback.
    std::string applicationName = "";

    gpu::PixelFormat colorFormat = gpu::PixelFormat::B8G8R8A8_UNORM;
    gpu::PixelFormat depthFormat = gpu::PixelFormat::D32_SFLOAT;
};

}
}
