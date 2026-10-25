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

#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace bg2e::draw { class Engine; }
namespace bg2e::scene { class Node; }

namespace bg2e::plugin {

using PluginId = uint64_t;
using SessionId = uint64_t;

struct PluginInfo {
    std::string id;
    std::string name;
    std::string description;
    std::string version;
    std::string provider;
    std::string url;
    std::string licenseExpression;
    std::string licenseText;
    std::string licenseUrl;
    std::string requiredSdkAbi;
    std::map<std::string, std::string> metadata;
};

enum class LoadStatus
{
    Loaded, LoadedWithWarnings, AlreadyLoaded, NotAPlugin, Incompatible,
    FilesystemFailure, LibraryLoadFailure, InitializationFailure,
    UnsupportedFile, WrongThread, Busy, OutOfMemory
};

struct LoadReport {
    LoadStatus status = LoadStatus::InitializationFailure;
    PluginId handle = 0;
    std::filesystem::path path;
    std::string pluginId;
    std::string message;
    std::vector<std::string> duplicates;
    std::vector<std::string> errors;

    bool loaded() const noexcept
    {
        return status == LoadStatus::Loaded || status == LoadStatus::LoadedWithWarnings
            || status == LoadStatus::AlreadyLoaded;
    }
};

struct ScanReport {
    LoadStatus status = LoadStatus::Loaded;
    std::string message;
    std::vector<LoadReport> files;
};

enum class UnloadReason
{
    Runtime, Shutdown, StartupFailure, InitializationFailure
};
enum class UnloadPolicy
{
    // Checks only the roots explicitly supplied by the caller.
    RefuseIfInUse,
    // Cleans only supplied roots; external ownership remains the caller's duty.
    RemoveComponents
};
enum class UnloadStatus
{
    Unloaded, InUse, Pending, NotFound, WrongThread, Busy, Failed, OutOfMemory
};

struct PluginServices {
    draw::Engine* graphics = nullptr;
    // True only at a host-owned stopped boundary, after workers and GPU users
    // have drained. Never set this while background scene parsing is active.
    bool stopped = false;
};

struct PluginLoadContext {
    PluginId handle = 0;
    std::string pluginId;
    std::filesystem::path path;
    PluginServices services;
};

struct PluginUnloadContext : PluginLoadContext {
    UnloadReason reason = UnloadReason::Runtime;
};

struct UnloadOptions {
    UnloadPolicy policy = UnloadPolicy::RefuseIfInUse;
    PluginServices services;
    // Optional known roots for RefuseIfInUse checks / RemoveComponents cleanup.
    // This list is NOT a complete lifetime census. The caller must explicitly
    // clean every active scene and release external references before unloading.
    std::vector<std::shared_ptr<scene::Node>> roots;
    // Invoked at the stopped boundary after preparation and before retirement.
    // Release history/clipboard/UI references and cancel/join application jobs.
    std::function<void()> releaseReferences;
};

struct UnloadReport {
    UnloadStatus status = UnloadStatus::Failed;
    PluginId handle = 0;
    std::string message;
};

namespace detail { struct ModuleState; }

// Optional host-owned lease for known asynchronous/external uses. This is not
// automatic component tracking. The programmer must release ALL plugin objects,
// weak control blocks and callbacks before unloading, even if no lease exists.
// Put leases in shared-object deleters/control blocks, not only base objects.
// Physical release happens at a host safe point outside every plugin stack.
class BG2E_API ModuleHandle {
public:
    ModuleHandle() = default;

    PluginId id() const noexcept;
    bool acceptingWork() const noexcept;
    explicit operator bool() const noexcept;

private:
    friend class PluginManager;
    friend class ComponentRegistrar;
    explicit ModuleHandle(std::shared_ptr<detail::ModuleState> state);
    std::shared_ptr<detail::ModuleState> _state;
};

}
