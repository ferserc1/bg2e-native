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

#include <bg2e.hpp>
#include <bg2e/ui/TextureWidgets.hpp>

class AppDelegate;

// Integrated lightmap baker settings and action window. The active model is
// the only target; the mode is fixed to RTAO.
class ModelLightmapWindow : public bg2e::ui::Window {
public:
    void init(AppDelegate * delegate);
    void cleanup();

    void setMessage(const std::string& message) { _message = message; }

private:
    AppDelegate * _appDelegate = nullptr;

    int _resolution = 512;
    int _frames = 16;
    int _samples = 8;

    std::string _message;
    bg2e::ui::TextureWidgets _preview;
    std::filesystem::path _previewPath;
};
