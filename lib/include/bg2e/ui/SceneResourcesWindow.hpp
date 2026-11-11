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

#include <bg2e/ui/Window.hpp>
#include <bg2e/db/SceneResourceManager.hpp>
#include <functional>

namespace bg2e::ui {

class BG2E_API SceneResourcesWindow : public Window {
  public:
    SceneResourcesWindow();

    // The host owns analysis lifetime and editor policy. Callbacks execute on the UI thread.
    void configure(std::function<const db::SceneResourceAnalysis*()> analysis,
        std::function<bool()> available, std::function<void()> analyze,
        std::function<void(bool)> cleanup, std::function<std::string()> status);
    void draw() override;

  private:
    std::function<const db::SceneResourceAnalysis*()> _analysis;
    std::function<bool()> _available;
    std::function<void()> _analyze;
    std::function<void(bool)> _cleanup;
    std::function<std::string()> _status;
    bool _ignoreWarnings = false;

    void drawContents();
};

}
