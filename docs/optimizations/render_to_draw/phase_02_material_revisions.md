# Phase 02 Material revisions and raster bindings

## Outcome

Implementation status: implemented and statically checked. Native checks, the optional verification probe, visual comparisons, and performance measurements have not run. Phase 01 and the shadow v3/indexed-UV entry validation remain pending as recorded in the summary.

Unchanged materials reuse packed shader data, and unchanged raster object/environment texture bindings are not reassigned each frame. Each acquired slot still receives all changes it has not consumed.

## Implementation

1. Add monotonic revisions for material values, realized image/sampler bindings, and classification changes. Setters that receive an unchanged effective value should not increment the relevant revision. Authored edits below the alpha threshold still update authored state even when the effective GPU state is unchanged.
2. Cover mutations through `materialAttributes()` references, whole-attribute assignment, `Drawable::material()`, cloning, texture realization, and sampler replacement. Track at the mutation source or introduce a compatible edit mechanism covering all callers. A flag only in `MaterialBase::setMaterialAttributes()` is insufficient. Keep existing pending-texture-realization flags distinct from renderer cache revisions.
3. Account for resource settings changed through referenced texture objects. Use resource identity/generation checks or a revision propagated by the existing realization path; pointer identity alone is insufficient if native state can change in place.
4. Cache `packMaterial()` output by material identity and consumed revisions. Validate newly changed state before publishing it. Preserve source identities for clones and independent materials with equal values.
5. In `ObjectDataBinding`, track each record's source identity, uniform contents, and texture/sampler generations. Update transforms or values when needed, and skip binding setters and `update()` when bindings are unchanged. Queue reorder or record replacement must invalidate the record correctly.
6. Apply equivalent binding-change detection to `EnvironmentDataBinding`, separating uniform changes such as sky blur from image/sampler changes.

Use revisions consumed independently by each frame slot and cache; never clear one shared dirty flag after the first consumer. If revision tracking touches shared base classes, preserve existing serialization and behavior outside the requested optimization.

## Verification and exit

Exercise each mutation route, delayed slot reuse, multiple consumers, unchanged assignments, clone independence, asynchronous texture completion where supported, environment changes, and queue reorder. Validate that invalid edited state cannot expose partially rebound resources.

After warmup, unchanged material pack counts and raster descriptor update counts should approach zero for unchanged records. Moving an object should update its transform without rebinding its textures. Exit with those reductions active in the functional renderer; revisions must already serve these consumers rather than exist only for later phases.

## Implementation record

- `base::MaterialAttributes` tracks authored edits in its setters and whole-value assignment. Equal assignments preserve its revision. The existing texture-realization flags retain their separate API and behavior for other rendering paths. Copies remain independent values, and revisions are not serialized.
- `base::Texture` tracks settings edits and whole-value assignment. Procedural generator dimension setters track changes; custom generators can call `markModified()` for other pixel-producing state. Settings propagate generator revisions when observed. Draw texture realization compares source identity and revision, including edits through retained references; assigning the same unchanged settings does not recreate GPU resources merely because a legacy pending flag was set. Explicit regeneration of custom procedural content uses `markModified()` followed by realization, or the existing explicit texture reload API.
- `draw::Texture` advances its binding revision after successful image/sampler replacement. Sampler updates compare effective native descriptions, including LOD clamping, and preserve authored settings without replacing an unchanged sampler. `gpu::Image::bindingGeneration()` advances through the existing layout-reset hooks used by Vulkan and Metal allocation/recreation/cleanup, so a stable wrapper pointer cannot hide a replaced native image/view. Samplers expose no in-place recreation API; retained ownership plus validity checks cover their immutable native state and replacement.
- `packMaterial()` now caches by each `MaterialBase` identity. Hits check source revisions, realized resource identities/generations, and color interpretation without reconstructing packed material values. Misses validate and pack a complete candidate before publishing effective value, binding, and alpha/transparency/solid classification revisions. Subthreshold alpha edits preserve authored state while leaving effective revisions unchanged. Clones start with separate caches and preserve unrealized source edits.
- Render-queue items snapshot material revisions, texture revisions, and image generations. Queue validation rejects a snapshot after in-place recreation or texture/sampler replacement. Each acquired object-binding slot independently retains its source identity, last uniform contents, consumed revisions/generations, and actual image/sampler owners. Stable records skip texture setters and descriptor updates; unchanged uniforms also skip uploads. Model movement updates uniforms without texture reassignment, and queue reorder/source replacement invalidates the affected records.
- Environment slots independently compare their four image/sampler bindings and generations. Sky blur/visibility changes update only the environment uniform. Resource snapshots retain native owners independently of later changes to a shared texture or environment wrapper.
- Full queue/resource validation precedes binding mutation. Slots are unpublished during updates; failures discard the affected resource set and its native pending writes, and retries restore all bindings. Previous slot owners survive until that slot is acquired again. No new waits, CMake edits, shader ABI changes, material merging, or scene material library were introduced.
- Diagnostic fallbacks are available through `MaterialBase::setMaterialCacheEnabled(false)` and `ObjectDataBinding::setReuseEnabled(false)` / `EnvironmentDataBinding::setReuseEnabled(false)`. They force packing or uniform/binding updates while retaining validation and acquired-slot ownership.

## Verification record

Performed: `git diff --check`; source assertions covering all authored fields in material/texture whole-value assignment, setter tracking, cache hit placement before packing, validation before publication, per-slot binding guards, and license headers. Audited Vulkan/Metal image reset paths, immutable sampler APIs, and resource-set binding/update behavior. These are static checks, not native test results.

Added an opt-in native probe to `examples/draw/10_deferred_renderer`: launch `draw_deferred_renderer` with `--verify-material-revisions` after an explicitly requested native build. It checks unchanged setters/texture assignments, direct attribute replacement, alpha-threshold transitions, invalid edits and recovery, sampler changes, direct realized-texture assignment, source texture/generator edits, pending clones, independent consumption, and diagnostic repacking. It runs during scene creation on an independent canopy material clone and leaves the normal scene intact. The probe has not been compiled or executed.

| Native scenario (pending) | Required evidence |
|---|---|
| Static scene after every acquired slot has warmed | `material.packs`, `objectBinding.descriptorUpdates`, and `environmentBinding.descriptorUpdates` approach zero; cache-hit counters continue advancing |
| Material value edit through `Drawable::material()` or a retained attributes reference | All reused slots receive the new values; image/sampler descriptors remain unchanged when resources are unchanged |
| Whole attributes replacement, equal assignments, and multiple consumers | Correct invalidation without revision rollback or consumption of a shared dirty flag |
| Transform-only animation | Object uniform uploads track movement; texture descriptor updates remain zero |
| Sampler replacement, texture reload, and in-place image recreation | Every slot detects replacement/generation changes; stale extracted queues are rejected |
| Referenced texture settings edits and custom generator `markModified()` | Packing rejects unrealized state; realization publishes complete resources before consumers resume |
| Delayed slot reuse, queue reorder, material replacement, aborted preparation | Changes reach each affected record; partially updated sets cannot be bound; retry restores full bindings |
| Environment blur/visibility, sampler replacement, and environment replacement | Uniform-only changes avoid descriptor updates; resource edits update only the affected bindings |
| Scene replacement, resize, cleanup, object export/import, clone edits and source-scene destruction | Correct output and independent ownership, with no validation errors or stale resources |
| Serialized asynchronous import completion where supported | Staged changes become visible only through the existing synchronized realization/publication path; no new concurrent mutation contract |

Counters added: `material.packs`, `material.packCacheHits`, `objectBinding.uniformUploads`, `objectBinding.descriptorUpdates`, `objectBinding.bindingCacheHits`, `environmentBinding.uniformUploads`, `environmentBinding.descriptorUpdates`, and `environmentBinding.bindingCacheHits`. These count successful packing/uploads/descriptor updates and reuse decisions; initialization can perform additional descriptor setup outside these counters. Record cold, warm, and transition frames separately and compare with the diagnostic fallback.

Outstanding: build/shader checks and the probe on Metal and Vulkan; all per-slot/visual/lifecycle/round-trip cases above; steady-state work-counter captures and exclusive CPU/GPU timings with the summary's scene metadata. No measured speedup or completed runtime acceptance is claimed.
