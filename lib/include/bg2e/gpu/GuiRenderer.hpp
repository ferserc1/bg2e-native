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

#include <bg2e/base/PlatformTools.hpp>
#include <bg2e/gpu/Instance.hpp>
#include <bg2e/gpu/PhysicalDevice.hpp>
#include <bg2e/gpu/Device.hpp>
#include <bg2e/gpu/WindowSurface.hpp>
#include <bg2e/gpu/CommandBuffer.hpp>
#include <bg2e/gpu/SurfaceFrame.hpp>
#include <bg2e/gpu/Sampler.hpp>
#include <memory>

namespace bg2e::gpu {

// Borrows GPU objects and the caller's current ImGui context. The owner must
// wait for GPU work, shut down this bridge, then destroy that context/device.
// No UI session, draw engine or native ImGui types cross this boundary.
class BG2E_API GuiRenderer {
  public:
    virtual ~GuiRenderer() = default;

    virtual void initialize(Instance&, PhysicalDevice&, Device&, WindowSurface&) = 0;
    virtual void prepareFrame(CommandBuffer*, SurfaceFrame*) = 0;
    virtual void draw(CommandBuffer&, SurfaceFrame&) = 0;
    virtual void shutdown() = 0;

    virtual uint64_t registerTexture(Image&, Sampler&) = 0;
    virtual void removeTexture(uint64_t identifier) = 0;
    virtual uint64_t textureGeneration() const = 0;
};

namespace vk {
BG2E_API std::unique_ptr<GuiRenderer> createGuiRenderer();
}
#ifdef BG2E_IS_MAC
namespace metal {
BG2E_API std::unique_ptr<GuiRenderer> createGuiRenderer();
}
#endif
}
