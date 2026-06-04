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

#include <bg2e/gpu/Instance.hpp>
#include <bg2e/gpu/PhysicalDevice.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/WindowSurface.hpp>
#include <bg2e/gpu/OffscreenSurface.hpp>
#include <bg2e/gpu/Common.hpp>

#include <cstdint>
#include <memory>

namespace bg2e {
namespace gpu {

enum class WindowType
{
    Vulkan,
    Metal
};

class BG2E_API Backend {
public:
    virtual ~Backend() = default;

    [[nodiscard]] virtual gpu::Instance* instance() const = 0;
    [[nodiscard]] virtual WindowType windowType() const = 0;

    [[nodiscard]] virtual std::unique_ptr<gpu::PhysicalDevice>  createPhysicalDevice()                                     const = 0;
    [[nodiscard]] virtual std::unique_ptr<gpu::Device>          createDevice()                                               const = 0;
    [[nodiscard]] virtual std::unique_ptr<gpu::WindowSurface>   createWindowSurface()                                        const = 0;
    [[nodiscard]] virtual std::unique_ptr<gpu::OffscreenSurface> createOffscreenSurface(const Size2D& size)                   const = 0;
};

}
}
