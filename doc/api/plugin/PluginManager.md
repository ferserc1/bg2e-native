# PluginManager

**Header:** `<bg2e/plugin/PluginManager.hpp>`

**Namespace:** `bg2e::plugin`

The host-owned manager owns library references, Plugin instances, metadata and
registration/lifecycle records. `get()` binds its owning thread on first access;
initialize/use it on the application's main thread. The singleton's host records
persist for the process, while native mappings are explicitly released on unload.

## Loading and scans

```cpp
static PluginManager& get();

LoadReport loadPlugin(const std::filesystem::path& path) noexcept;
LoadReport loadPlugin(const std::filesystem::path& path,
    std::vector<std::string>& duplicates) noexcept;
ScanReport loadPlugins(const std::filesystem::path& directory,
    bool missingIsEmpty = false) noexcept;
ScanReport loadPlugins(const std::filesystem::path& directory,
    std::vector<std::string>& duplicates, bool missingIsEmpty = false) noexcept;
```

### Explicit file and directory loading

```cpp
#include <bg2e/plugin/PluginManager.hpp>
#include <iostream>

// Acquire/use the manager on the main thread; modulePath and pluginDirectory
// are application-owned filesystem paths. Graphics initialization is not needed.
auto& manager = bg2e::plugin::PluginManager::get();
const auto file = manager.loadPlugin(modulePath);
if (!file.loaded())
{
    std::cerr << file.message << '\n';
    for (const auto& error : file.errors)
    {
        std::cerr << error << '\n';
    }
}
const auto directory = manager.loadPlugins(pluginDirectory);
for (const auto& report : directory.files)
{
    // Inspect report.status, report.handle, report.duplicates and report.errors.
}
```

`Plugin::loadPlugin`/`loadPlugins` provide equivalent facades; only the manager
exposes the optional `missingIsEmpty` scan argument. Empty/default-directory scans
are not an explicit-load fallback for a deployment mistake in the tutorial.

File loading validates a regular-file path, platform extension, canonical path,
exports, descriptor, metadata and registration. Canonical-path duplicates return
the existing active ID; duplicate stable plugin IDs also return `AlreadyLoaded`
and roll back the newly created candidate instance. Retired records do not replace
an active module. Loaded metadata must have nonempty ID and display name.

Directory loading is nonrecursive. Candidates are sorted by filesystem path
before individual attempts; one recoverable file failure does not stop later files.

| Platform | Candidate suffixes |
|----------|--------------------|
| Linux | `.so` (not versioned `.so.1` filenames). |
| Windows | `.dll`, compared case-insensitively. |
| macOS | `.dylib`, `.so`, `.bundle`. |

Missing explicit directories return `FilesystemFailure` unless `missingIsEmpty`
is true. The default automatic scan uses true. Other filesystem failures are
reported. A raw library lacking `getPluginInstance` is `NotAPlugin`, not a startup
exception. Missing `getPluginApi` on a candidate exporting the factory is
`Incompatible`; inspect descriptor mismatch details in `errors`.

No-vector overloads warn for omitted duplicates. The vector overloads append all
duplicate attempt names without printing those warnings. Neither form clears the
caller's vector. The aggregate ScanReport status describes enumeration/operation
failure; inspect **each** `files` entry for actual loading outcomes, even when the
aggregate status is `Loaded`.

Public loading catches recoverable loader/API exceptions. Under allocation failure,
only a status code may be available. An allocation failure while appending caller
diagnostics can happen after a module has loaded; a failure status is not a promise
that all public-call side effects were reversed. Inspect the nonzero ID/manager
state when available. Native faults cannot be caught by this API.

## Queries and scene cleanup

```cpp
const PluginInfo* info(PluginId id) const;
ModuleHandle module(PluginId id) const;
std::vector<PluginId> plugins() const;
std::vector<std::string> componentTypes(PluginId id) const;
size_t removeComponents(PluginId id, scene::Node* root);
```

| Member | Result |
|--------|--------|
| `info(id)` | Host-owned copied metadata, or null for unknown/failed IDs or wrong thread. A historical successful record can still have metadata after retirement; this is not proof of an active instance. |
| `module(id)` | Explicit lease for a nonretired module; empty for missing/retired IDs or wrong thread. |
| `plugins()` | Nonretired IDs in load order; empty on the wrong thread. No owning Plugin pointer is exposed. |
| `componentTypes(id)` | Host-owned copy of committed component IDs; empty when no instance remains. Wrong-thread calls throw `logic_error`. |
| `removeComponents(id, root)` | Loaded-instance scene cleanup. Throws for invalid/retired IDs, wrong thread, reentrancy, active readers or failed component callbacks/allocation. Null root returns zero for a valid loaded ID. |

The matching instance/static Plugin helpers are described in [Plugin](Plugin.md).
Scene cleanup is required for **every** active root before unload; it does not
release external ownership. Do not retain raw metadata or executable callbacks
past owner removal.

## Unload and deferred collection

```cpp
UnloadReport unload(PluginId id, const UnloadOptions& options = {}) noexcept;
void collect() noexcept;
```

`unload` requires `options.services.stopped == true` at an actual host-owned stopped
boundary. That flag is a caller guarantee, not a request to wait for jobs/GPU work.
Clean every scene and all external objects/control blocks/callbacks first. Unknown
uses are not counted and may crash after unload; read the complete
[unload contract and sequence](unloading.md).

On success, the manager retires owned contributions, finalizes/destroys the Plugin
instance and releases its native loader reference. Known explicit ModuleHandle
leases yield `Pending` after retirement; release them and call `collect` outside
all plugin stacks with producers stopped. Collection attempts retired modules in
reverse order and never bypasses known leases, reader guards or failed cleanup.
There is no forced-unload or guaranteed hot-reload facility.

## Background registry readers

```cpp
std::shared_ptr<void> registryReadLease();
```

Acquire on the manager thread outside callbacks **before** launching background
scene parsing. Hold the returned host token until every registry read and related
worker capture destructor has completed. A live reader makes loading/unloading
return `Busy`. MainLoop's `asyncLoad` acquires one automatically. This reader token
is separate from a module lease; it protects registry mutation, not arbitrary
plugin object lifetimes. Direct unguarded concurrent registry mutation is outside
the contract.

## Session integration

```cpp
SessionId beginSession();
ScanReport discover(SessionId session) noexcept;
void stopSession(SessionId session) noexcept;
void prepareSession(SessionId session, PluginServices services, UnloadReason reason) noexcept;
void retireSession(SessionId session) noexcept;
void finishSession(SessionId session, UnloadReason reason) noexcept;
```

These are host integration operations used by MainLoop, not additional author
callbacks. `beginSession` returns a nonzero session ID and throws `logic_error` on
wrong-thread, reentrant or reader-blocked use. `discover` scans the default path
with missing-is-empty semantics; modules first created by that scan belong to the
session. Existing explicit loads are **not adopted**, so automatic cleanup cannot
silently unload someone else's module.

The void operations ignore wrong-thread/reentrant/zero-session calls. For a valid
session they visit its modules in reverse load order. Stop callbacks and lifecycle
callbacks are one-shot; retirement/finalization are idempotent. `finishSession`
provides fallback preparation/retirement and uses no graphics services. Physical
release is a subsequent collection step. Custom hosts must supply the same ordering
and programmer-owned cleanup guarantees as [MainLoop lifecycle](lifecycle.md).
