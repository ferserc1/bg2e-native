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

#include <bg2e/render/Engine.hpp>

namespace bg2e {

namespace app {

class MainLoop;

}

namespace ui {

class UserInterface;

class BG2E_API UserInterfaceDelegate {
    friend class app::MainLoop;
public:
    virtual void init(bg2e::render::Engine*, UserInterface*) {}
    virtual void drawUI();
    
    inline uint32_t uiWidth() const { return _viewportWidth; }
    inline uint32_t uiHeight() const { return _viewportHeight; }
    
protected:
    uint32_t _viewportWidth = 0;
    uint32_t _viewportHeight = 0;
};

}
}
