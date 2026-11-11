# SceneResourceManager

`<bg2e/db/SceneResourceManager.hpp>` provides file-only resource analysis and
explicit cleanup. No scene graph, component factories, engine, or device is
created. SDK callers may pass either a scene JSON file or a directory resolved
as `Name/Name.json`. Existing scene-loading APIs still accept JSON files.

```cpp
auto analysis = bg2e::db::SceneResourceManager::analyze(sceneFile);
// Present analysis.unusedFiles and analysis.warnings to the user.
auto plan = bg2e::db::SceneResourceManager::createCleanupPlan(analysis);
auto result = bg2e::db::SceneResourceManager::executeCleanup(plan);
```

If dependency warnings exist, plan creation requires explicit acknowledgement:
`createCleanupPlan(analysis, true)`. This can delete files required by unknown
components or unreadable/unsupported model dependencies. Fatal errors reading the
main document or enumerating its directory throw; partial deletion failures are
returned in `SceneCleanupResult::errors` alongside successful deletions.

The scene directory is an exclusive resource container: every regular file not
reachable from the main JSON is a cleanup candidate, regardless of extension.
Nested directories are inspected, but directories and symbolic links are never
removed and directory links are not followed. The main JSON and recognized
resources are protected. External dependencies are reported separately and never
removed. Paths are resolved relative to the scene document or containing model,
and resolved resource paths are deduplicated.

The shared `sceneFileReferences()` traversal is also used by loader texture
counting. BG2 dependency expansion uses `meshTexturePaths()` from
`<bg2e/db/mesh_bg2.hpp>`, sharing the existing header/material reader with
`countMeshTextures()`. The path API retains repeated material slots, reads no
image pixels or geometry, and throws on malformed/unreadable metadata. The legacy
count API retains its zero-on-error behavior. Supported references include BG2
material images (including AO/GI lightmaps), environment images, embedded text
material images, fonts, and font notices. Unknown component types produce
warnings instead of being silently treated as resource-free.

Analysis records file contents, sizes, timestamps, dependencies, and the local
inventory. Plan creation and execution independently reanalyze to reject stale or
modified snapshots before any deletion. No new candidates are added during
execution. Callers must serialize all writers to the scene directory during
execution; revalidation is not a filesystem transaction or an interprocess lock.

## Composer policy

The SDK has no saved/dirty-state policy. Composer queues resource operations at
its stopped document IO boundary, settles pending edits, and saves a new/dirty
scene before publishing analysis. Editing, pending commands, an active
transaction, document replacement, or another save invalidates the displayed
snapshot. Imports and lightmap baking prevent admission of resource operations.

Complete SDK saves intentionally preserve authored live source paths. Therefore
Composer transactionally reloads the saved snapshot before cleanup, using its
existing scene loading/publication path. This rebinds live sources to packaged
files and clears selection, the old undo/redo history, and the material clipboard
before deletion. If loading fails, no files are removed. The plan is revalidated
again after successful loading. This also protects future saves of fonts,
environments, and text materials whose original source became an orphan.

The UI is `bg2e::ui::SceneResourcesWindow`, available under
Window > Scene Resources Manager. It displays relative candidate paths, sizes,
total space, external resources, dependency warnings, an explicit warning
override, and a final deletion confirmation. Ordinary saves never clean files.

See [the CPU probe and manual Composer checks](../../../tests/db/scene_resources/README.md).
