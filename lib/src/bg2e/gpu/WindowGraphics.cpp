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

#include <bg2e/gpu/WindowGraphics.hpp>
#include <bg2e/base/PlatformTools.hpp>
#include <SDL2/SDL_vulkan.h>
#ifdef BG2E_IS_MAC
#include <SDL2/SDL_metal.h>
#endif
#include <stdexcept>

namespace bg2e::gpu {
Size2D drawableSize(SDL_Window* window, BackendType backend)
{
    if (!window)
    {
        throw std::invalid_argument("Drawable size requires a window.");
    }
    int width = 0, height = 0;
    if (backend == BackendType::Vulkan)
    {
        SDL_Vulkan_GetDrawableSize(window, &width, &height);
    }
#ifdef BG2E_IS_MAC
    else if (backend == BackendType::Metal)
    {
        SDL_Metal_GetDrawableSize(window, &width, &height);
    }
#endif
    else
    {
        throw std::invalid_argument("Drawable size requires an available graphics backend.");
    }
    return {uint32_t(width > 0 ? width : 0), uint32_t(height > 0 ? height : 0)};
}
}
