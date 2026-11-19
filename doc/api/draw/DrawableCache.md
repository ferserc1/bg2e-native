# DrawableCache and resource usage

Headers: `<bg2e/draw/DrawableCache.hpp>`, `<bg2e/draw/DrawableUsage.hpp>`;
BG2 integration: `<bg2e/db/drawable_cache.hpp>`.

Obtain `Engine::drawableCache()` by reference or create a shared context with
`Engine::createDrawableCache()`. Both require a live initialized Device. Stop
frame/background producers and use the serialized resource-update thread for
mutations. The service has no Composer/tree dependency.

## Entry operations

| Operation | Result and purpose |
|---|---|
| `load(path, Loader)` | `DrawableCacheResult`; normalized BG2/VWGLB reuse, supplied DB loader on a miss. |
| `db::loadCachedDrawableBg2(path, cache, onTextureLoaded)` | DB adapter returning the same structured result. An Engine overload uses its default cache. |
| `find(path)`, `findIdentifier(identifier)` | Lookup without loading. |
| `registerProcedural(resource, displayName)` | Register a complete resource with unique identity. |
| `registerImported(resource, sourcePath, displayName)` | Unique imported identity with source provenance. |
| `replace(handle, resource)` | Replace realization while preserving identity/aliases/export bookkeeping; obey exclusive-owner constraints. |
| `refresh(handle)` | Publish in-place owner geometry changes and fresh originals. |
| `entries()`, `inspect(handle)` | Value metadata, independent original definitions, revision, paths and logical usage. |
| `canUnload(handle)`, `unload(handle)`, `clear()` | Explicit residency release; reject active logical uses. |

Check result diagnostics before using handles. Diagnostics distinguish invalid
arguments, absent/foreign resources, cross-device use, ownership/collision,
busy state and load/resource failures. Failed publication preserves the prior
entry. `resolve()` is the internal owner/render bridge, not mutable instance API.

`setExportPath()`, `recordExportPublication()` and `addLookupAlias()` are separate
operations. Published submesh bases support instance export rebasing. Export paths
do not automatically become source aliases. `restoreOriginalDefinitions(handle,
history)` requires the matching retained History realization already restored.

## Explicit lifetimes

A `DrawableResourceHandle` is nonowning; copies do not register use. It exposes
`valid()`, `identifier()`, `geometryRevision()` and `context()`. Replacement keeps
identity; unload/engine cleanup invalidates it.

`retain(handle, DrawableUse, debugIdentifier)` returns `DrawableUsageResult` with
a move-only lease. Categories are SceneOwner, SceneInstance, History, Job and
Frame. Require meaningful debug identifiers. History/Job/Frame leases pin the
acquired realization; a frame lease lasts until GPU completion. Optional retain
overloads accept a nonthrowing device-release hook and an independent dependency
reader. Capture weak payloads in release/read callbacks, not raw component pointers.

The cache strongly retains unused entries. `queryUsage()` reports logical counts,
not `shared_ptr::use_count()`. Clear refuses the entire operation if any entry is
retained. Keep Engine alive through native consumers; forced shutdown releases
tracked raster/BLAS/material realizations before Device teardown.

## Cleanup snapshots

`scopeIdentity()`, `contentGeneration()` and `dependencies()` expose transient
CPU metadata for runtime disk protection. Content changes advance generation;
lease/frame turnover does not. `DrawableDependencies` contains files, models and
warnings; `appendMaterialDependencies()` includes all filesystem texture slots,
including lightmaps. Use these values through
[SceneResourceManager](../db/SceneResourceManager.md), not as persisted cache data.

See [architecture](../../architecture/drawable_caches_and_instances.md) for
originals, owner replacement, document scopes and offscreen reuse.
