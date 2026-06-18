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

#include <string>
#include <mutex>

namespace bg2e {
namespace ui {

class BG2E_API Loader {
public:
    void setMessage(const std::string& msg);
    std::string getMessage() const;

    void setProgress(float progress);   // clamped to [0, 1]
    float getProgress() const;

    // Call once per ImGui frame while the loader is active.
    // Draws a centered non-closeable window with message + progress bar.
    void draw();

private:
    mutable std::recursive_mutex _mutex;
    std::string _message  = "Loading...";
    float       _progress = 0.f;
};

} // ui
} // bg2e
