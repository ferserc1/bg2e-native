# Scene resource manager checks

`probe.cpp` is a standalone C++20 probe linked against bg2e. It requires no engine,
window, GPU device, image decoder, or component registration. No CMake files were
changed. The implementing agent has not compiled or executed this probe.

Run `scene_resources_probe EXISTING_DISPOSABLE_DIRECTORY`. The probe creates a
unique child directory and performs cleanup only within its generated scene.
Fixtures are retained for inspection, including on failure.

Coverage: BG2 material image paths (including repeated albedo/AO slots), loader
count compatibility, nested nodes, legacy model names, text material images,
fonts and notices, deduplication, direct JSON and directory inputs, external
resources, stale snapshots with same-size/same-timestamp content edits, explicit
unknown-dependency overrides, malformed BG2 models, and directory symlinks where
supported.

## Composer manual checks

1. Open Scene selects a directory and opens `Name/Name.json`. Selecting an invalid
   directory reports an error and preserves the current document. SDK direct JSON
   opening remains available.
2. Open Window > Scene Resources Manager. Its button reads Analyze scene resources.
   A freshly opened, unchanged scene is analyzed directly without rewriting its
   JSON or sidecars (compare file sizes/timestamps before and after). A new/dirty
   scene must save before any analysis appears. Cancel or fail that save: no
   analysis is published.
3. Add then remove a drawable, replace a texture, and save. Analyze: old exported
   sidecars appear as candidates, current models/images/fonts/notices do not.
4. Select an orphan image as a new material source without saving. Analyze: the
   scene is saved first and the source survives through its exported reference.
5. Edit after analysis, including an active field edit or queued resource command.
   The old result must not permit deletion; the stopped IO boundary checks again.
6. Delete: review the exact list and explicit warning override, cancel and verify
   no changes, then confirm. The saved scene is reloaded before deletion; selection, undo/redo and the material clipboard are cleared.
   Cleanup is never invoked by ordinary saving.
7. Mutate a scene/model/candidate on disk after analysis. Deletion must reject the
   stale plan and request another analysis. Imports/bakes must not overlap cleanup.
8. Exercise missing/unsupported dependencies: warnings appear, deletion requires
   explicit acknowledgement, and external files and symlinks are preserved.

9. Save a scene using an orphan HDR, font, font notice, or text material image as
   an authored source. Clean and immediately save again: the second save must
   succeed. The cleanup reload must rebind those live references to the packaged
   copies before removing the original sources.
