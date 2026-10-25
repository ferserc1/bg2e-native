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

#include <bg2e/plugin/Plugin.hpp>

#include <memory>

namespace bg2e::plugin {

// Main-thread API. First access binds the manager to the calling thread.
// Public recoverable-error boundaries have an allocation-free status fallback.
class BG2E_API PluginManager {
public:
    static PluginManager& get();

    LoadReport loadPlugin(const std::filesystem::path& path) noexcept;
    LoadReport loadPlugin(const std::filesystem::path& path,
        std::vector<std::string>& duplicates) noexcept;
    ScanReport loadPlugins(const std::filesystem::path& directory,
        bool missingIsEmpty = false) noexcept;
    ScanReport loadPlugins(const std::filesystem::path& directory,
        std::vector<std::string>& duplicates, bool missingIsEmpty = false) noexcept;

    const PluginInfo* info(PluginId id) const;
    ModuleHandle module(PluginId id) const;
    std::vector<PluginId> plugins() const;

    // Acquire on the manager thread before launching background scene parsing.
    // Keep the lease in the worker until all registry reads have completed.
    // Loading/unloading returns Busy while readers are active. MainLoop's
    // asyncLoad acquires this automatically.
    std::shared_ptr<void> registryReadLease();

    // Cleanup facade for clients using managed IDs instead of Plugin pointers.
    // Throws on invalid IDs, wrong thread, busy state or component callback errors.
    size_t removeComponents(PluginId id, scene::Node* root);
    std::vector<std::string> componentTypes(PluginId id) const;

    // CALLER CONTRACT: clean every scene and release every external plugin use
    // before requesting unload. Unknown uses are NOT detected or counted.
    // Incorrect use causes undefined behavior/crashes. Run only at a host-owned
    // stopped boundary, outside plugin stacks; see doc/api/plugin/unloading.md.
    UnloadReport unload(PluginId id, const UnloadOptions& options = {}) noexcept;

    // MainLoop owns only modules first discovered in this session. Explicit
    // modules are not silently adopted or unloaded by automatic scanning.
    SessionId beginSession();
    ScanReport discover(SessionId session) noexcept;
    void stopSession(SessionId session) noexcept;
    void prepareSession(SessionId session, PluginServices services, UnloadReason reason) noexcept;
    void retireSession(SessionId session) noexcept;
    void finishSession(SessionId session, UnloadReason reason) noexcept;

    // Host safe-point only, under the same caller cleanup contract as unload.
    // Retries physical release of retired modules when explicit leases are gone.
    // Unknown components, weak control blocks and callbacks are NOT detected.
    void collect() noexcept;

private:
    PluginManager();
    ~PluginManager();
    struct Impl;
    std::unique_ptr<Impl> _impl;
};

}
