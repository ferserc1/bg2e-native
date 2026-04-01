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
#include <bg2e/render/vulkan/common.hpp>
#include <SDL2/SDL.h>

namespace bg2e {
namespace render {
namespace vulkan {

class Instance;

class BG2E_API Surface {
public:
    void create(const Instance& instance, SDL_Window* window);
    void cleanup();

    VkExtent2D getExtent() const;

    inline VkSurfaceKHR handle() const { return _surface; }
    inline bool isValid() { return _surface != VK_NULL_HANDLE; }

protected:
    VkSurfaceKHR _surface = VK_NULL_HANDLE;
    VkInstance _instance = VK_NULL_HANDLE;
    SDL_Window* _window = nullptr;
};

}
}
}
