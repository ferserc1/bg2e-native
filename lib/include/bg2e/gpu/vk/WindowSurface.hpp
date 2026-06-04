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

#include <bg2e/gpu/WindowSurface.hpp>
#include <bg2e/gpu/vk/common.hpp>

#include <SDL2/SDL.h>

namespace bg2e {
namespace gpu {
namespace vk {

class WindowSurface : public gpu::WindowSurface {
public:
    void create(gpu::Instance* instance) override;
    void cleanup() override;

    uint32_t width() const override;
    uint32_t height() const override;

    bool isValid() const override;

    VkSurfaceKHR handle() const;
    SDL_Window* sdlWindow() const;

private:
    VkSurfaceKHR _surface{VK_NULL_HANDLE};
    VkInstance _vkInstance{VK_NULL_HANDLE};
    SDL_Window* _window{nullptr};
};

}
}
}
