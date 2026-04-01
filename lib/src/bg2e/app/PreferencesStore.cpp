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

#include <bg2e/app/PreferencesStore.hpp>

namespace bg2e::app {

PreferencesStore& PreferencesStore::instance()
{
    static PreferencesStore store;
    return store;
}

Preferences& PreferencesStore::preferences()
{
    return preferences("");
}

Preferences& PreferencesStore::preferences(const std::string& scope)
{
    std::lock_guard<std::mutex> lock(_mutex);
    
    auto it = _prefs.find(scope);
    if (it != _prefs.end())
    {
        return *(it->second);
    }
    
    auto prefs = scope == "" ?
        std::make_unique<Preferences>() :
        std::make_unique<Preferences>(scope);
    Preferences& ref = *prefs;
    _prefs[scope] = std::move(prefs);
    return ref;
}

void PreferencesStore::saveAll()
{
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto& kv : _prefs) {
        kv.second->save();
    }
}


}
