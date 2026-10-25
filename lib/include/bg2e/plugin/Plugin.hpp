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

#include <bg2e/plugin/Sdk.hpp>
#include <bg2e/plugin/Types.hpp>

namespace bg2e::plugin {

class ComponentRegistrar;

class BG2E_API Plugin {
public:
    virtual ~Plugin();

    virtual PluginInfo info() const = 0;

    virtual void registerComponents(ComponentRegistrar&, const PluginLoadContext&) = 0;

    virtual void onPrepareUnload(const PluginUnloadContext&)
    {
    }
    virtual void onUnload(const PluginUnloadContext&)
    {
    }

    // Successfully registered IDs only. Failed/duplicate contributions are absent.
    const std::vector<std::string>& componentTypes() const noexcept;

    // Main-thread stopped boundary. Call for EVERY active scene before unload;
    // includes disabled nodes. Removes membership, not external shared ownership.
    // Release detached objects, clones, history, weak control blocks, callbacks
    // and jobs too. Violating the unload contract causes undefined behavior and
    // may crash the application. See doc/api/plugin/unloading.md.
    size_t removeComponents(scene::Node* root) const;
    static size_t removeComponents(PluginId plugin, scene::Node* root);

    static LoadReport loadPlugin(const std::filesystem::path& path) noexcept;
    static LoadReport loadPlugin(const std::filesystem::path& path,
        std::vector<std::string>& duplicates) noexcept;
    static ScanReport loadPlugins(const std::filesystem::path& directory) noexcept;
    static ScanReport loadPlugins(const std::filesystem::path& directory,
        std::vector<std::string>& duplicates) noexcept;

private:
    friend class PluginManager;
    std::vector<std::string> _componentTypes;
};

}

// Use once per module, at namespace scope. No plugin objects or registrations
// are created by static initializers. Factory failures become null instances.
#define BG2E_DEFINE_PLUGIN(PluginClass) \
    extern "C" BG2E_PLUGIN_EXPORT void BG2E_PLUGIN_CALL bg2eDestroyPlugin(void* value) noexcept \
    { \
        try \
        { \
            delete static_cast<bg2e::plugin::Plugin*>(value); \
        } \
        catch (...) \
        { \
        } \
    } \
    extern "C" BG2E_PLUGIN_EXPORT const bg2e::plugin::PluginApi* BG2E_PLUGIN_CALL getPluginApi() noexcept \
    { \
        static const bg2e::plugin::PluginApi api = [] \
        { \
            auto result = bg2e::plugin::detail::localSdk(); \
            result.destroy = &bg2eDestroyPlugin; \
            return result; \
        }(); \
        return &api; \
    } \
    extern "C" BG2E_PLUGIN_EXPORT void* BG2E_PLUGIN_CALL getPluginInstance() noexcept \
    { \
        try \
        { \
            return static_cast<bg2e::plugin::Plugin*>(new PluginClass()); \
        } \
        catch (...) \
        { \
            return nullptr; \
        } \
    }
