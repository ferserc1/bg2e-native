//
//  PreferencesStore.cpp
//  bg2e
//
//  Created by Fernando Serrano Carpena on 6/11/25.
//

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
