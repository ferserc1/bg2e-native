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

#include <bg2e/app/Preferences.hpp>
#include <bg2e/math/linear_snapping.hpp>

namespace bg2e_composer::workspaces::common {

class SnappingPreferences {
  public:
    SnappingPreferences();
    explicit SnappingPreferences(bg2e::app::Preferences& preferences);

    bg2e::math::LinearSnapSettings settings() const;
    bg2e::math::LinearSnapGesture beginGesture() const;

    // Invalid edits leave the last valid settings in memory and in storage.
    bool setSettings(bg2e::math::LinearSnapSettings settings);

  private:
    bg2e::app::Preferences& _preferences;
    bg2e::math::LinearSnapSettings _settings;
};

}
