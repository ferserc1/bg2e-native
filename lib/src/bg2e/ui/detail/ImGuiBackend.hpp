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

#include <bg2e/ui/UserInterface.hpp>
#include <memory>

namespace bg2e::ui::detail {

// UI owns backend integration. No GPU type depends on ImGui or ui.
class ImGuiBackend {
public:
    virtual ~ImGuiBackend() = default;
    virtual void initialize(render::Engine&) = 0;
    virtual void initialize(draw::Engine&) = 0;
    virtual void prepareFrame(gpu::CommandBuffer*, gpu::SurfaceFrame*) = 0;
    virtual void draw(VkCommandBuffer, VkImageView) = 0;
    virtual void draw(gpu::CommandBuffer&, gpu::SurfaceFrame&) = 0;
    virtual void shutdown() = 0;
};

std::unique_ptr<ImGuiBackend> createVulkanImGuiBackend();
#ifdef BG2E_IS_MAC
std::unique_ptr<ImGuiBackend> createMetalImGuiBackend();
#endif

}
