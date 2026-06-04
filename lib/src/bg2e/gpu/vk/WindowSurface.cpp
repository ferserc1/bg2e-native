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

#include <bg2e/gpu/vk/WindowSurface.hpp>
#include <bg2e/gpu/vk/Instance.hpp>

#include <SDL2/SDL_vulkan.h>

namespace bg2e {
namespace gpu {
namespace vk {

void WindowSurface::create(gpu::Instance* instance)
{
    auto* vkInst = dynamic_cast<vk::Instance*>(instance);
    _vkInstance = vkInst->vkInstanceHnd();
    _window = instance->window();
    SDL_Vulkan_CreateSurface(_window, _vkInstance, &_surface);

    int w, h;
    SDL_GetWindowSize(_window, &w, &h);
    _width = static_cast<uint32_t>(w);
    _height = static_cast<uint32_t>(h);
}

void WindowSurface::cleanup()
{
    if (_surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(_vkInstance, _surface, nullptr);
        _surface = VK_NULL_HANDLE;
    }
}

uint32_t WindowSurface::width() const
{
    int w, h;
    SDL_GetWindowSize(_window, &w, &h);
    return static_cast<uint32_t>(w);
}

uint32_t WindowSurface::height() const
{
    int w, h;
    SDL_GetWindowSize(_window, &w, &h);
    return static_cast<uint32_t>(h);
}

bool WindowSurface::isValid() const
{
    return _surface != VK_NULL_HANDLE;
}

VkSurfaceKHR WindowSurface::handle() const
{
    return _surface;
}

SDL_Window* WindowSurface::sdlWindow() const
{
    return _window;
}

}
}
}
