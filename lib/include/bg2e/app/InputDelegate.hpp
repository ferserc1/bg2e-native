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

#include <bg2e/app/KeyEvent.hpp>

#include <filesystem>

namespace bg2e {
namespace app {

class InputDelegate {
public:
    virtual void keyDown([[maybe_unused]] const KeyEvent& keyEvent) {}

    virtual void keyUp([[maybe_unused]] const KeyEvent& keyEvent) {}

    virtual void mouseMove([[maybe_unused]] int x, [[maybe_unused]] int y) {}

    virtual void mouseButtonDown([[maybe_unused]] int button, [[maybe_unused]] int x, [[maybe_unused]] int y) {}

    virtual void mouseButtonUp([[maybe_unused]] int button, [[maybe_unused]] int x, [[maybe_unused]] int y) {}

    virtual void mouseWheel([[maybe_unused]] int deltaX, [[maybe_unused]] int deltaY) {}

    virtual void fileDropped([[maybe_unused]] const std::filesystem::path &) {}
};

}
}
