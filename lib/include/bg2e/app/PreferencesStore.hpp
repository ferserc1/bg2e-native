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

#include <bg2e/app/Preferences.hpp>

#include <unordered_map>
#include <mutex>
#include <memory>

namespace bg2e {
namespace app {

class BG2E_API PreferencesStore {
public:
    static PreferencesStore& instance();

    // Global preferences → empty scope
    Preferences& preferences();

    // Scoped preferences (ej: "ui", "render", "editor_layout")
    Preferences& preferences(const std::string& scope);

    // Save all stores. Note that it is not necesary to call this function, because
    // al Preferences instances will be saved on their destructor. But maybe it's
    // a good idea to save the preferences sometimes to prevent lost them in case of
    // an application crash
    void saveAll();

private:
    PreferencesStore() = default;

    PreferencesStore(const PreferencesStore&) = delete;
    PreferencesStore& operator=(const PreferencesStore&) = delete;

    std::unordered_map<std::string, std::unique_ptr<Preferences>> _prefs;
    std::mutex _mutex;
};

}
}
