# Step 2 — Icon resources and permanent path cache

Prerequisite: step 1 accepted. Read summary/progress and inspect the extracted infrastructure.

## Work

1. Add public `ui::Icon` and `ui::IconManager`, exported through `ui/all.hpp`. Use opaque implementations with no native or ImGui identifiers in public APIs. Expose `UserInterface::icons()` and `IconManager::load(const std::filesystem::path&)` returning a shared icon resource. Provide minimal validity/fallback and source information as useful; size belongs to drawing.
2. Own the manager in UserInterface::Impl, tied to its engine/context. Permit use during delegate initialization once renderer and registry exist; do not gate it only on the ready flag, which is set after delegate initialization. Reset cache ownership before registry shutdown; invalidate surviving handles on cleanup and isolate a new UI session from old handles.
3. Use one cache indexed by absolute, lexically normalized native filesystem paths. Resolve relative paths against the working directory at resolution time and document that policy. Do not lowercase paths, serialize them through lossy narrow strings, require files to exist for key formation or promise symlink deduplication. Catch filesystem errors from asset resolution.
4. Reuse `db::loadImage` and `draw::Texture` upload APIs with RGBA, linear filtering, clamp-to-edge and no mipmaps. Inspect current UI backend color/blending conventions before selecting the icon color interpretation. Reuse shared registered-image ownership instead of adding another descriptor cache. Retain only necessary resources, not a permanent second cache of decoded CPU pixels.
5. Cache successful resources strongly until session cleanup. Cache missing/corrupt/unsupported-source outcomes too, logging once per key. Keep resource size/tint independent of the key. A failed source retains its own source identity even if it shares the fallback image.
6. Create a small question-mark fallback using procedural CPU pixels or an existing suitable repository asset, with no external download or image atlas. Reuse one fallback image per session. If fallback texture creation itself fails, retain a safe no-texture state for a text question-mark rendering path; never retry failed asset I/O every frame. Distinguish recoverable asset failures from device/session failures rather than swallowing every backend exception.
7. Add private active-session resolution keyed to ImGui context for later static widgets. Remove mappings during all cleanup and initialization-failure paths. No public singleton or global permanent cache.

## Deliverable boundary

Explicit resource loading and lifecycle are usable; no automatic preload operation or demand scan. No widget/toolbar/menu additions yet. No changes to general texture caches. Inspect source and ownership paths only.

## Handoff

Record normalization, error policy, color choice and initialization order in progress. Report implementation and await validation. Next: [step 3](step-03-image-and-button-wrappers.md), path-first drawing APIs.
