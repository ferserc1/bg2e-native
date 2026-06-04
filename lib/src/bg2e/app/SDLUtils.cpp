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

#include <bg2e/app/SDLUtils.hpp>

#ifdef BG2E_LINUX

#include <SDL2/SDL.h>
#include <wayland-client.h>
#include <cstring>
#include <cstdlib>

static bool hasSdlWaylandDriver()
{
    const int numDrivers = SDL_GetNumVideoDrivers();
    for (int i = 0; i < numDrivers; ++i)
    {
        const char* driver = SDL_GetVideoDriver(i);
        if (driver && std::strcmp(driver, "wayland") == 0)
        {
            return true;
        }
    }
    return false;
}

static bool canConnectToWayland()
{
    wl_display* display = wl_display_connect(nullptr);
    if (!display)
    {
        return false;
    }
    wl_display_disconnect(display);
    return true;
}

#endif

void bg2e::app::initSdlVideoDriver()
{
#ifdef BG2E_LINUX
    if (hasSdlWaylandDriver() && canConnectToWayland())
    {
        setenv("SDL_VIDEODRIVER", "wayland", 1);
    }
#endif
}
