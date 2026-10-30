# Draw Composer import lifecycle

The draw application's [SceneImporter](../../../apps/bg2e_composer/src/SceneImporter.cpp) owns a joinable CPU reader and up to 16 waiting local jobs. Shared `db::readGltf`, `readObj`, `loadMeshBg2` and `readSceneDocument` prepare CPU data; graph factories, image decoding and GPU realization run on the stopped main-thread path. CPU parsing is not background GPU loading or an immutable snapshot of external image files.

Request IDs, document generations, renderer generations and weak destination/reimport identities serve different purposes. Publication checks them independently; reusing a renderer still requires the current document generation. Cancelled/replaced jobs cannot publish into another document. Canonical source paths locate reimport wrappers by identity and preserve their current parent. Ordinary failed replacement restores graph order, camera/environment selection, dirty state and renderer binding. Binding recovery failure propagates to renderer-first teardown.

The localhost server admits one HTTP import until completion; `/status` stays responsive while the accepted handler waits. Service stop fails/wakes its request before joining handlers. No HTTP handler touches graph/GPU state. Shutdown retires bake work, stops/joins CPU and HTTP readers, detaches UI/settings, and releases renderer/extraction before scene resources and Engine teardown. Cancellation is cooperative around CPU parser calls; synchronous realization cannot be interrupted.

Open Scene has a separate guarded document operation: MainLoop worker IO/JSON, document-generation checked completion, then one component realization per Loader frame before adoption. MainLoop joins its workers before normal/exceptional graphics cleanup and expires exit completion tokens. This worker lifecycle is shared infrastructure; Composer's scene/import/publication policies remain app-owned.

Composer scene edits, import adoption and the user-visible Undo/Redo menu are
described in the [command/memento architecture guide](../../architecture/command-memento/index.md).

See [complete resource IO](../../api/db/DrawResourceIO.md), [safe updates](../../safe_update_scene.md) and [manual import checks](../../../tests/draw/composer_import/README.md). Backend/HTTP execution is lead-owned; no new execution result is asserted here.
