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

#include <bg2e/draw/RenderSettingsPreferences.hpp>
#include <bg2e/ui/GraphicsSettingsWindow.hpp>
#include <bg2e/ui/UISettingsWindow.hpp>

#include <memory>

namespace bg2e_composer { class AppDelegate; }

namespace bg2e_composer::workspaces::common {

class SettingsWindows {
  public:
    explicit SettingsWindows(const std::shared_ptr<AppDelegate>& delegate);

    void init();

    bg2e::ui::GraphicsSettingsWindow& graphics();
    void openInterface();
    void drawInterface();

  private:
    std::shared_ptr<bg2e::draw::RenderSettingsPreferences> _preferences;
    bg2e::ui::GraphicsSettingsWindow _graphics;
    bg2e::ui::UISettingsWindow _uiSettings;
};

}
