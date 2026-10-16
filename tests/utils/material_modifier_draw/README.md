# M02-04 draw MaterialModifier checks

The agent does not compile/run these checks. No CMake target is added. The lead
can compile/link main.cpp against the rebuilt engine as a standalone CPU fixture;
it does not initialize an Engine or GPU. It checks indexed/exact-name/group/
regex-search dispatch, valid no-op counts, invalid targets/schema, explicit zero,
alpha cutoff/UV, ignored invalid/null fields, CPU texture paths, callbacks and
clone descriptor independence.

For live checks, use a loaded draw Drawable in the deferred example at a stopped
scene/GPU boundary. The M02-07 manipulation example will provide final UI presets.
Compile the engine and existing production MaterialModifier callers, then check
Metal and Vulkan where available:

1. Set materialDefinition(0).metalness to0.2 and live drawMaterial(0)'s metalness
   to0.8. Apply roughness-only to the Drawable. Both roughness values change;
   metalness must remain0.2 in the definition and0.8 live. Repeat for alphaCutoff,
   albedo alpha, UV/scales and explicit zero/false. material() aliases live state
   after loading and is not the independent CPU definition.
2. Patch a draw MaterialBase reference and shared pointer. Only live attributes
   change; an owning Drawable's CPU definition remains untouched. Null shared
   pointers/invalid indices/names/groups return false/zero. A valid empty or
   all-ignored patch returns true/counts each matched submesh.
3. Apply one albedo texture to a loaded Drawable. CPU and live descriptors should
   be distinct objects with the resolved path. Other CPU/live scalar values and
   descriptor identities remain unchanged. Existing clean GPU texture slots
   retain their realization. updateTextures flushes any previously pending slots
   using the live material's existing cache policy; this is not a per-slot-only
   update or a rollback transaction.
4. Confirm callback behavior: CPU-only application calls none. Live realization
   calls once for each non-null descriptor loaded by updateTextures, including
   other pending slots. Already realized clean slots do not callback. A callback
   may run before replacement is committed. Do not reenter/mutate the same target
   or retain references to a callback's stack.
5. Give CPU/live fields different values and apply exact-name/group/regex patches;
   metadata, transforms, visibility and unmatched submeshes remain unchanged.
   Duplicate names affect the first exact match; groups affect all matches;
   regex uses regex_search. Counts include valid no-op patches.
6. Use an invalid texture path and a throwing callback. Exceptions propagate;
   CPU/live attribute edits already applied remain, earlier submeshes may have
   changed, old GPU texture owners remain until a successful update. Correct the
   path/callback and retry. Redraw/reset history even after partial failure.
7. Clone a realized Drawable, patch only its scalar/texture properties, and verify
   original CPU/live descriptors, images, samplers and attributes stay independent.
   Existing clone behavior snapshots live values into clone definitions; it does
   not preserve a source CPU/live divergence as two distinct clone definitions.
8. Queue changes through DefaultRenderLoopDelegate::enqueueRendererUpdate with
   renderer generation plus weak node/component/Drawable identity checks. A fresh
   scene frame rebuilds queue uniforms, texture bindings and query alpha tables.
   Reset RendererDeferred::resetUpscaler after material changes (also on partial
   failure) before the next frame; custom renderers use their own reset hooks.
   Do not retain Renderer pointers or run modifications during update/record/UI
   draw callbacks. Manual loops stop scene/frame producers, then explicitly
   request a scene frame/history reset. Fully paused scenes retain their image.
9. With a checker albedo, modify alphaCutoff, albedo alpha, alpha UV/scale and
   transparency flags. Raster and query-shadow cutouts should follow the next
   extracted snapshot. Test no-RT/shadows-off and normal/reduced/available FSR
   output. The existing eight distinct cutout image/sampler-pair limit remains.
10. Check reload/updateMaterials preserve patched live fields without copying the
    CPU definition over unrelated live edits. cleanup retains existing behavior:
    it snapshots live attributes back into definitions. Later initial loading
    uses definitions; CPU definition edits alone do not replace existing materials.
    Run production material_modifier gallery regressions, including callbacks.
