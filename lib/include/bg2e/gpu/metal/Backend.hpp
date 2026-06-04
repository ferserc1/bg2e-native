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

#include <bg2e/gpu/Backend.hpp>
#include <bg2e/gpu/metal/common.hpp>

#include <bg2e/gpu/metal/Instance.hpp>

#include <memory>

namespace bg2e {
namespace gpu {
namespace metal {

class BG2E_API Backend : public gpu::Backend {
public:
    gpu::Instance* instance() const override;
    gpu::WindowType windowType() const override;

    std::unique_ptr<gpu::PhysicalDevice>  createPhysicalDevice()                           const override;
    std::unique_ptr<gpu::Device>          createDevice()                                   const override;
    std::unique_ptr<gpu::WindowSurface>   createWindowSurface()                            const override;
    std::unique_ptr<gpu::OffscreenSurface> createOffscreenSurface(uint32_t width,
                                                                   uint32_t height)        const override;

private:
    mutable std::unique_ptr<Instance> _instance;
};

}
}
}