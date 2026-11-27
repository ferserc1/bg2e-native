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

#include "SnappingPreferences.hpp"

#include <bg2e/app/PreferencesStore.hpp>

namespace bg2e_composer::workspaces::common {

SnappingPreferences::SnappingPreferences()
    : SnappingPreferences(bg2e::app::PreferencesStore::instance().preferences("draw.bg2e_composer.snapping"))
{
}

SnappingPreferences::SnappingPreferences(bg2e::app::Preferences& preferences)
    : _preferences(preferences)
{
    using bg2e::math::LinearSnapMode;
    _settings.enabled = preferences.get<bool>("enabled", _settings.enabled);
    if (preferences.get("mode", std::string("relative")) == "absolute")
    {
        _settings.mode = LinearSnapMode::Absolute;
    }
    auto loaded = _settings;
    loaded.step = preferences.get<double>("linearStep", _settings.step);
    if (loaded.valid())
    {
        _settings = loaded;
    }
}

bg2e::math::LinearSnapSettings SnappingPreferences::settings() const
{
    return _settings;
}

bg2e::math::LinearSnapGesture SnappingPreferences::beginGesture() const
{
    return bg2e::math::LinearSnapGesture(_settings);
}

bool SnappingPreferences::setSettings(bg2e::math::LinearSnapSettings settings)
{
    if (!settings.valid())
    {
        return false;
    }

    _preferences.set<bool>("enabled", settings.enabled);
    _preferences.set("mode", settings.mode == bg2e::math::LinearSnapMode::Absolute ? "absolute" : "relative");
    _preferences.set<double>("linearStep", settings.step);
    _settings = settings;
    return true;
}

}
