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

#include <bg2e/app/Mouse.hpp>
#include <SDL2/SDL.h>

namespace bg2e {
namespace app {

bool Mouse::leftButtonPressed() {
    int x, y;
    return (SDL_GetMouseState(&x, &y) & SDL_BUTTON_LMASK) != 0;
}

bool Mouse::middleButtonPressed() {
    int x, y;
    return (SDL_GetMouseState(&x, &y) & SDL_BUTTON_MMASK) != 0;
}

bool Mouse::rightButtonPressed() {
    int x, y;
    return (SDL_GetMouseState(&x, &y) & SDL_BUTTON_RMASK) != 0;
}

int Mouse::x() {
    int x, y;
    SDL_GetMouseState(&x, &y);
    return x;
}

int Mouse::y() {
    int x, y;
    SDL_GetMouseState(&x, &y);
    return y;
}

}
}
