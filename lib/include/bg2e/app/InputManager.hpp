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
#include <bg2e/app/InputDelegate.hpp>
#include <bg2e/app/KeyEvent.hpp>
#include <bg2e/math/base.hpp>

#include <memory>

namespace bg2e {
namespace app {

class BG2E_API InputManager {
public:
    struct MouseButtonsStatus {
        bool left;
        bool middle;
        bool rigth;
        
        uint32_t x;
        uint32_t y;
    };
    static MouseButtonsStatus getMouseStatus();
    
    static glm::vec2 normalizedCursorPosition(uint32_t viewportWidth, uint32_t viewportHeight);
    
    void keyDown(const KeyEvent& event);
    void keyUp(const KeyEvent& event);
    void mouseMove(int x, int y);
    void mouseButtonDown(int button, int x, int y);
    void mouseButtonUp(int button, int x, int y);
    void mouseWheel(int deltaX, int deltaY);

    inline void setDelegate(std::shared_ptr<InputDelegate> delegate) { _delegate = delegate; }

protected:
    std::shared_ptr<InputDelegate> _delegate;
};

}
}
