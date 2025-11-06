//
//  PreferencesStore.hpp

#pragma once

#include <bg2e/common.hpp>

#include <bg2e/app/Preferences.hpp>

#include <unordered_map>
#include <mutex>
#include <memory>

namespace bg2e {
namespace app {

class PreferencesStore {
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
