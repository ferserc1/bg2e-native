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

#include "SettingsWindows.hpp"
#include "../../AppDelegate.hpp"

#include <bg2e/app/PreferencesStore.hpp>

#include <utility>

namespace bg2e_composer::workspaces::common {

SettingsWindows::SettingsWindows(const std::shared_ptr<AppDelegate>& delegate)
{
    auto& store = bg2e::app::PreferencesStore::instance().preferences("draw.bg2e_composer.graphics");
    _preferences = std::make_shared<bg2e::draw::RenderSettingsPreferences>(&store);
    _preferences->setAutomaticPersistence(true);
    _preferences->setExcludeSceneSettingsFromAutomaticPersistence(true);
    _preferences->onSceneEdits([owner = std::weak_ptr<AppDelegate>(delegate), preferences = std::weak_ptr<bg2e::draw::RenderSettingsPreferences>(_preferences)]
        (auto changes, auto completion)
    {
        if (auto active = owner.lock())
        {
            active->editSceneSettings(preferences.lock(), std::move(changes), std::move(completion));
        }
        else if (completion)
        {
            completion({false, {{"target_expired", "", "Composer is no longer available"}}});
        }
    });
    _preferences->connect(delegate); // Before the renderer's first attachment.
    _graphics.bind(_preferences);
    _graphics.setSceneEditInteractionsEnabled(true);
    _graphics.setPosition(320, 80);
    _graphics.setSize(620, 740);
    _graphics.close();
}

void SettingsWindows::init()
{
    _uiSettings.init(true, true);
    _uiSettings.setToolbarSettingsVisible(false);
    _uiSettings.close();
}

bg2e::ui::GraphicsSettingsWindow& SettingsWindows::graphics()
{
    return _graphics;
}

void SettingsWindows::openInterface()
{
    _uiSettings.open();
}

void SettingsWindows::drawInterface()
{
    if (_uiSettings.isOpen())
    {
        _uiSettings.draw();
    }
}

}
