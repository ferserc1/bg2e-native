# Composer loading correction and tracing-capacity investigation

**Later capacity expansion:** [the lead-selected 16384 policy](tracing-capacity-16384.md)
now supersedes the old 256/128 profile diagnosis as an implementation restriction.
The explanation below records why the original failure occurred; current native
tables use exact device budgets and v2 shaders. The neutral-texture fallback and
per-object resource ownership remain in force.

The same capacity correction now covers direct SoftShadowPass (reserves `(2,1,9)`,
shader interface v2, Vulkan bindings16/24 and Metal argument records5/6). This
investigation's initial eight-pair diagnosis is historical, not the current cap.
See `tracing-capacity-16384.md` for implementation and validation details.

## Subsequent lead-selected correction: neutral missing-texture fallback

The lead explicitly selected recoverable sampled-table overflow to validate scene
loading before redesigning image/sampler management. TracingSceneResources now
admits image/sampler pairs atomically; if either new entry cannot fit, both material
indices become `UINT32_MAX` and `fallbackTextureSlots` increments. No table grows
past its compiled capacity. Existing admitted pairs still resolve after saturation.
Shared GLSL/Metal sampling helpers return white, or a flat normal for slot3, without
descriptor access or sRGB decoding. Scalars/tints/alpha remain unchanged and raster
materials are untouched. No geometry, light or effect is disabled. Missing cutout
texture alpha is one; authored scalar alpha/cutoff still applies.

A warning is emitted on entering overflow per service and rearmed after successful
recovery or clear(), avoiding per-frame log spam. Native/profile limits and future
canonicalization/bindless work are unchanged. The CPU/GPU record sizes stay the
same; compatible custom tracing shaders must implement the sentinel. Static checks
and an extended manual RTAO overflow/recovery fixture are delivered; no shader/C++
compilation or GPU/app execution. Rebuild both native tracing shader libraries
before opening the supplied scene with GI/reflections enabled.

The original analysis below is historical where it describes the old throwing
policy or says a remedy is not selected; this explicit lead decision supersedes it.

## Scope and evidence

The project lead reports synchronous Open and a macOS application exit:
`Tracing material capacity exceeded: images=129, samplers=129, capacity=128`.
Static inspection only; no build, application, shader or GPU execution. Existing
null-entry corrections are preserved. Tracing behavior is analyzed, not changed,
pending the lead's choice. [Progress](../progress.md) remains authoritative.

## Implemented loading correction

Production `apps/bg2e_composer/src/AppDelegate.cpp` wraps
`MainLoop::asyncLoad` with `AppDelegate::asyncLoadGuarded`; its toolbar performs
scene loading with Loader callbacks. Draw Composer previously had no equivalent
guard and performed Open synchronously in `processDocumentIo`.

Draw Composer now implements the same guarded entry point and uses the shared
MainLoop pause/Loader/completion mechanism for scene document IO and raw JSON
parsing. Worker captures own detached JSON only, not the stage, active graph,
factory registry or device. The stopped main-thread completion verifies document
generation and queues validated CPU graph construction. A second Loader phase
realizes one component's resources per frame while scene rendering is paused;
the UI is presented between components. All components must succeed before tool
preparation/adoption. Dirty Save/Discard/Cancel runs once, before background work.
Errors discard the candidate and preserve the previous document before adoption.
Input, imports publication, overlapping IO and ordinary Close are guarded while
loading. JSON/legacy scene extensions and canonical scene folders use this path;
model Open and the existing separate async import service retain their own paths.

The shared MainLoop previously detached its workers, which could access the
Loader/MainLoop after exceptional shutdown. It now owns joinable workers, joins
before normal or exceptional execution cleanup, and invalidates queued completion
tokens of the exiting run. Thread-start failure restores scene/UI presentation.

This does **not** claim fully asynchronous GPU upload. UI and renderer use the
same draw device/queues; blindly copying production's entire worker load would
introduce unsafe concurrent GPU/cache/graph access. Individual component/model
load, image decode/upload, tool creation and renderer/environment initialization
can still block their frame. Fully streaming model/image decode and bounded
upload slices need additional CPU preparation APIs/scheduling; these remaining
costs must not be described as eliminated.

Lead checks: [Composer IO matrix](../../../../tests/draw/composer_io/README.md),
including window responsiveness, error/dirty preservation and shutdown, on
available Metal/Vulkan. Isolate the loading check from the tracing failure by
explicitly disabling advanced tracing during that check.

## What the tracing error actually means

1. `lib/src/bg2e/draw/pipeline/TracingSceneResources.cpp` builds independent image
   and sampler tables from **all six realized slots** of each participating
   material. It deduplicates actual shared resource pointers, not equal image
   bytes or equal sampler descriptions. Upon inserting the 129th distinct entry,
   it throws `std::length_error` before publishing the new snapshot. **129 is the
   first overflowing count, not the full scene's required count.**
2. The M03 design deliberately chose fixed compiled `32/128/256` shader/layout
   profiles rather than runtime-sized/bindless descriptor tables. Both backends
   have these engine limits. See [accepted M03 design](../milestone_03_ray_tracing/rt-design.md#material-resources-alternatives-and-recommended-limits).
3. `lib/src/bg2e/gpu/metal/Device.cpp` declares a **conservative engine ceiling**
   of 256 sampled images and `min(256, maxArgumentBufferSamplerCount())` samplers
   for Tier-2 argument buffers, plus 1024 total stage resources. These numbers are
   not a complete report of the native GPU's actual argument-buffer capacity.
4. `ResourceArrayCapabilities::profileCapacity` in `gpu/Common.hpp` subtracts
   reserved effect/environment resources, then picks one common image/sampler
   profile. GI calls `profileCapacity(5,3,15)` and reflections `(6,3,16)`.
   Thus 256 is **always excluded for these Metal effects by the declared image
   ceiling**: 256+5 (or +6) exceeds 256. Even with a native sampler budget of at
   least 256, 256+3 would exceed the declared sampler ceiling too. 128 is the next
   compiled profile. The lead's hardware model/native limits are not supplied.
5. `draw/MaterialBase.cpp` creates separate neutral textures for absent slots;
   `Texture::colorTexture` allocates a new image and sampler each time. Materials
   without authored maps therefore consume entries too. Independent clones can
   consume more entries by design. Filesystem cache reuse does not canonicalize
   all fallback/clone resources. This explains the potential inflation, but the
   exact contribution of each supplied model was not measured.
6. `RendererDeferred::update` invokes GI/reflection `prepare` during frame
   preparation. Capacity exceptions are not converted to a recoverable UI status
   there. This happens after document parsing/adoption, outside DocumentIo's
   candidate-load catch, explaining the top-level application exit. It is a
   missing graceful error policy, not evidence of a native Metal RT crash.

These are sampled-material resource limits, **not** limits of 128 triangles,
objects, rays, bounces or recursion. Metal advanced effects use compute intersection
queries; Vulkan uses native RT stages for GI/reflections. There is no evidence in
this log that Metal intersection traversal itself failed.

## Options (not implemented in this correction)

| Option | Benefit | Requirements / limits |
|---|---|---|
| **Graceful optional-effect failure + table canonicalization (recommended first)** | Stop application exits; substantially reduce redundant resource entries without dropping geometry or changing images | Catch a specific capacity error at an appropriate transactional/frame boundary; show required/available counts and an explicit warning/status. Choose retain old scene vs opt-in whole-effect raster/IBL fallback. Canonicalize equal sampler descriptions and provably immutable neutral resources in tracing snapshots. Preserve independent editable material/clone owners and invalidate on mutation; do not alias arbitrary mutable images by filename. This does not remove the ceiling for genuinely many unique textures. |
| Correct Metal capacity reporting and select larger supported profiles | Make already-compiled 256 profile usable on hardware that truly supports its complete layout | Report defensible native total budgets, reserve effect bindings correctly, validate per-stage/argument-buffer constraints and sampler counts, and test Metal resource residency/layouts. Do not simply claim unsupported limits or replace 128 with 256. Additional profiles or separate image/sampler capacities require coordinated shader/layout/API changes. |
| Runtime-sized or larger bindless resource tables | Scales to much larger scenes without atlas resampling | Backend capability/design work, shader ABI variants or specialization, residency, immutable snapshots and explicit native limits. Vulkan feature requirements differ from Metal Tier-2 argument buffers. No implementation can promise unlimited resources. |
| Texture arrays/atlases or material paging | Reduce simultaneous descriptor demand / handle larger working sets | Arrays/atlases require format/dimension/mip/wrap/filtering fidelity; paging needs globally correct closest-hit/cutout ordering and resource lifetime. Naively tracing each material batch independently is incorrect. Highest complexity relative to this failure. |
| Temporary explicit IBL/raster-only use | Allows scene-loading validation immediately | Disable GI, reflections and any other full-table effect such as advanced AO. Separate query shadows have their own small cutout-table/fallback contract. This is a user-selected workaround, not a tracing fix or permission to silently change defaults. |

Recommended sequence: graceful failure/diagnostics plus canonicalization first;
then correct capacity reporting/profile coverage based on actual device budgets.
If real authored resource counts still exceed supported profiles, select a larger
table architecture explicitly. Never truncate occluders, drop textured slots,
substitute arbitrary textures or silently clamp counts.
