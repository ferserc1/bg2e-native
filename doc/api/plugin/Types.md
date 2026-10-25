# Plugin metadata, contexts, reports and handles

**Header:** `<bg2e/plugin/Types.hpp>`

**Namespace:** `bg2e::plugin`

## Identifiers

```cpp
using PluginId = uint64_t;
using SessionId = uint64_t;
```

`PluginId` is the process-local managed load/contribution identity returned in a
report's `handle`; zero means no identity is available. `PluginInfo::id` is instead
a persistent author-chosen string. SessionId identifies a host discovery session.
Do not serialize numeric load/session IDs or confuse them with component type IDs.

## PluginInfo

All fields below are `std::string` except `metadata`, which is
`std::map<std::string, std::string>`:

| Field | Meaning |
|-------|---------|
| `id` | Required nonempty stable plugin identity; active duplicates are not registered twice. |
| `name` | Required nonempty display name. |
| `description` | Human-readable purpose. |
| `version` | Plugin's own version, independent from C contract/SDK ABI versions. |
| `provider`, `url` | Author/provider name and project/provider URL. |
| `licenseExpression` | Explicit license identifier/expression, such as the example's `GPL-3.0-or-later`. |
| `licenseText`, `licenseUrl` | Optional license text and canonical explanatory link. |
| `requiredSdkAbi` | Optional additional exact SDK ABI requirement. Empty adds no metadata restriction; descriptor compatibility is still mandatory. |
| `metadata` | Extensible author-defined string key/value information. |

The host copies metadata before registration and does not infer a plugin's license
from the engine's license. Authors must supply their plugin's actual license
information. The loader validates nonempty identity/name; it does not parse plugin
version ranges, license expressions or arbitrary metadata. Do not use
`requiredSdkAbi` to replace or weaken the generated descriptor checks.

## Services and contexts

```cpp
struct PluginServices {
    draw::Engine* graphics = nullptr;
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
```

The ID/path strings identify the current plugin and its canonical module path.
`graphics` is borrowed and optional. It is null during registration; plugins must
not assume that loading implies a GPU. `stopped` reports the host boundary's actual
work state; supplying true for runtime unload is a programmer guarantee that
workers/frame production are stopped and GPU work drained, not a synchronization
operation performed by the struct. Context references/borrowed services must not
be retained beyond their valid callback/service lifetime.

| `UnloadReason` | Meaning |
|----------------|---------|
| `Runtime` | Explicit runtime retirement/unload. |
| `Shutdown` | Successful automatic application-session shutdown. |
| `StartupFailure` | Automatic session cleanup after startup/execution failure. |
| `InitializationFailure` | Rollback of a failed candidate factory/metadata/registration attempt. |

Availability and callback order are detailed in [Lifecycle](lifecycle.md).

## LoadReport and LoadStatus

```cpp
struct LoadReport {
    LoadStatus status = LoadStatus::InitializationFailure;
    PluginId handle = 0;
    std::filesystem::path path;
    std::string pluginId;
    std::string message;
    std::vector<std::string> duplicates;
    std::vector<std::string> errors;
    bool loaded() const noexcept;
};
```

`loaded()` is true for `Loaded`, `LoadedWithWarnings` and `AlreadyLoaded` only.
`path` is the requested/canonical path when available; `pluginId` is the stable
string when metadata was obtained. `duplicates` records each omitted duplicate
attempt, while `errors` includes ABI mismatch details or registration validation
failures. `message` supplies the primary diagnostic when available.

| Status | Meaning |
|--------|---------|
| `Loaded` | Instance initialized and contributions committed without registration diagnostics. Zero contributions is valid. |
| `LoadedWithWarnings` | Initialization completed with duplicates or supported invalid-contribution diagnostics; inspect both diagnostic vectors. |
| `AlreadyLoaded` | Canonical path or stable ID already has an active load; `handle` identifies that existing record. |
| `NotAPlugin` | No `getPluginInstance` export. A raw dynamic library may be valid for another purpose. |
| `Incompatible` | Missing descriptor query, unsupported descriptor size/version, SDK/ABI mismatch or incompatible metadata requirement. |
| `FilesystemFailure` | Missing/unreadable path, invalid path kind, canonicalization or enumeration failure. |
| `LibraryLoadFailure` | Native loader could not open/resolve the module's dependencies. |
| `InitializationFailure` | Descriptor query/factory/metadata/registration failure, including null instance or invalid required identity/name. |
| `UnsupportedFile` | Explicit regular file has an unsupported platform extension. Directory scans omit such names. |
| `WrongThread` | Operation attempted outside the manager thread. |
| `Busy` | Reentrant manager operation or active background registry readers (also invalid zero session for discovery). |
| `OutOfMemory` | Allocation failure; optional path/string/vector details may be unavailable. |

Recoverable loading returns reports instead of propagating plugin exceptions.
A nonzero handle on a failure is diagnostic context, not proof of an active plugin.
Allocation failure can also occur while appending diagnostics after successful
loading; inspect manager state if a handle is available. Native memory faults,
`abort` and terminating/static-initializer faults are not recoverable reports.

## ScanReport

```cpp
struct ScanReport {
    LoadStatus status = LoadStatus::Loaded;
    std::string message;
    std::vector<LoadReport> files;
};
```

Inspect every `files` entry. The aggregate `status` reports directory/operation
failure, not a reduction of all per-file statuses. It can remain `Loaded` when a
candidate is incompatible or is `NotAPlugin`. Directory iteration failure can
still leave reports for already enumerated candidates. Allocation failure can
interrupt accumulation; do not assume every filesystem entry has a report.

## UnloadOptions and UnloadReport

```cpp
struct UnloadOptions {
    UnloadPolicy policy = UnloadPolicy::RefuseIfInUse;
    PluginServices services;
    std::vector<std::shared_ptr<scene::Node>> roots;
    std::function<void()> releaseReferences;
};

struct UnloadReport {
    UnloadStatus status = UnloadStatus::Failed;
    PluginId handle = 0;
    std::string message;
};
```

| Policy | Scope |
|--------|-------|
| `RefuseIfInUse` | Refuses when matching components are found under the **supplied** roots. With no roots it trusts the programmer's prior cleanup. |
| `RemoveComponents` | Removes matching components under supplied roots before retirement. It cannot release external ownership or discover omitted scenes. |

`releaseReferences` is an optional host-owned cleanup callback after preparation
and optional scene removal, before retiring contributions. It must release any
plugin-owned captures/references too; otherwise the callback object retained by
the caller could itself outlive the library.

| UnloadStatus | Meaning |
|--------------|---------|
| `Unloaded` | The manager's native loader reference has been released; other native loader owners may still map the file. |
| `InUse` | Matching components remain in the supplied roots under refusal policy. No complete lifetime census is implied. |
| `Pending` | Known explicit leases delay native release after retirement, or a retired record cannot yet be collected. |
| `NotFound` | No record has this numeric ID. |
| `WrongThread` | Not the manager thread. |
| `Busy` | Reentrancy, background registry readers, or no caller-guaranteed stopped boundary. |
| `Failed` | Cleanup/callback failure; do not treat it as a completed unload. |
| `OutOfMemory` | Allocation failure during the operation; inspect partial state before further work. |

The [unload contract](unloading.md) is mandatory for both policies. Logical
retirement can precede physical release; never keep using retired Plugin instances
or services merely because a module mapping remains.

## ModuleHandle

```cpp
ModuleHandle() = default;
PluginId id() const noexcept;
bool acceptingWork() const noexcept;
explicit operator bool() const noexcept;
```

A default handle is empty (`id() == 0`, false). Copies share host-owned module
state; `PluginManager::module(id)` and `ComponentRegistrar::module()` provide
leases. `acceptingWork()` becomes false when preparation stops new work. Boolean
conversion means a state is held, not that a Plugin instance is still alive or
that new work is allowed.

A live explicit lease delays physical native release. Release/reset all copies
when the protected work ends. For plugin-created shared objects, keep any lease
in the deleter/control block through weak-reference destruction; an object-only
base member can disappear too early. Physical release happens at a later host
safe point, never from a lease destructor or plugin destructor stack.

Leases are optional known-use protection, not automatic component tracking or a
replacement for the programmer's cleanup obligation. They do not keep the Plugin
instance, scene or graphics engine alive after retirement. The registry-reading
token from `registryReadLease()` is a separate host token with different semantics.
