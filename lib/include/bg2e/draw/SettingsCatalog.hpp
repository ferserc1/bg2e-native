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
#include <bg2e/reflection/TypeInfo.hpp>
#include <map>
#include <any>
#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace bg2e::draw {
struct SettingHandle {
    uint64_t generation = 0;
    std::string component;
    std::string property;
};
struct SettingDescriptor {
    SettingHandle handle;
    std::string componentType;
    reflection::PropertyInfo property;
    bool available = true;
};
struct SettingsTarget {
    std::string id;
    reflection::TypeInfo type;
    void* instance = nullptr;
    std::shared_ptr<void> owner; // Optional lifetime owner; no settings value copy.
    std::function<bool(const std::string&)> available;
    std::function<void()> changed; // Nonthrowing post-commit invalidation/rebuild request.
    // Validate proposed values merged with current live getter values, before writes.
    std::function<void(const std::map<std::string, std::any>&)> validate;
};
struct SettingsIssue {
    std::string code;
    std::string path;
    std::string message;
    bool error = true;
};
struct BG2E_API SettingsResult {
    bool applied = false;
    std::vector<SettingsIssue> issues;
    bool valid() const;
};
struct SettingChange {
    SettingHandle handle;
    std::any value;
};
// Main-thread target/metadata view, usable without Engine, MainLoop or UI.
// Clear/rebind before any borrowed target dies. No value/profile cache is kept.
class BG2E_API SettingsCatalog {
  public:
    SettingsCatalog() = default;
    SettingsCatalog(const SettingsCatalog&) = delete;
    SettingsCatalog& operator=(const SettingsCatalog&) = delete;
    SettingsCatalog(SettingsCatalog&&) = delete;
    SettingsCatalog& operator=(SettingsCatalog&&) = delete;
    void reset(std::string rendererId = {});
    void add(SettingsTarget target);
    uint64_t generation() const;
    const std::string& rendererId() const;
    std::vector<SettingDescriptor> properties(bool includeUnavailable = false) const;
    SettingHandle handle(const std::string& component, const std::string& property) const;
    SettingDescriptor describe(const SettingHandle& handle) const;
    std::any read(const SettingHandle& handle) const;
    SettingsResult validate(const std::vector<SettingChange>& changes) const;
    SettingsResult apply(const std::vector<SettingChange>& changes);
    // Exact any contract, finite numeric components, ranges and enum identities.
    static bool isDebugSetting(const SettingDescriptor& descriptor);
    static bool isGraphicsSetting(const SettingDescriptor& descriptor);

    static void validateValue(const reflection::PropertyInfo& property, const std::any& value);
  private:
    friend class SettingsCodec;
    const SettingsTarget& target(const SettingHandle& handle) const;
    uint64_t _generation = 0;
    std::string _renderer;
    std::vector<SettingsTarget> _targets;
};
}
