# Text components progress

## Resume here

- Planning: complete.
- Implementation: step 1 implemented; steps 2–8 not started.
- Current milestone: **1 — Generic double-sided materials**, awaiting user acceptance.
- Next milestone: **2 — Texture source editing and export metadata**, only after acceptance/continuation.
- Next action: wait for the user's library/Composer build and manual feedback. Fix any reported step-1 failures before advancing. If the user accepts or asks to continue without reporting failure, read [step-02-texture-metadata.md](step-02-texture-metadata.md), then inspect `lib/include/bg2e/base/Texture.hpp` and the current source-edit/export paths.
- Blockers: compilation and Vulkan/Metal runtime behavior remain unverified, as required by the execution contract.
- Builds/application checks: not performed; exclusively user-owned.
- Font choice: Didact Gothic Regular, OFL 1.1; asset provenance/license work belongs to step 3.

## Milestone ledger

| Step | Description | Implementation | User acceptance |
| --- | --- | --- | --- |
| 1 | Generic double-sided materials | Implemented | Awaiting user build/manual acceptance |
| 2 | Texture source edit/export metadata | Not started | Pending |
| 3 | Font resources, stb and licensed asset | Not started | Pending |
| 4 | Text data and procedural rasterization | Not started | Pending |
| 5 | TextComponent | Not started | Pending |
| 6 | Scene persistence and packaging | Not started | Pending |
| 7 | Composer integration | Not started | Pending |
| 8 | API docs and interactive tutorial | Not started | Pending |

## Mandatory handoff record

Append a record after each implementation milestone and update the resume block/ledger. Preserve earlier feedback and decisions.

### Step N — Title

- Status: in progress / implemented awaiting user acceptance / accepted / blocked.
- Changed files:
- Delivered behavior and API decisions:
- Source review performed (not a build):
- Composer manual checks to communicate to the user:
- If not observable in Composer: explicitly state that user compilation of library and Composer is sufficient for this milestone.
- User-reported result and acceptance evidence:
- Known limits, blockers and unfinished work:
- Next milestone and its purpose:
- Exact next action / file to resume:

Do not mark implementation or acceptance complete based on this template. No build/test execution is authorized by this plan.

### Step 1 — Generic double-sided materials

- Status: implemented awaiting user acceptance.
- Changed files:
  - `lib/include/bg2e/base/MaterialAttributes.hpp`
  - `lib/src/bg2e/base/MaterialAttributes.cpp`
  - `lib/src/bg2e/utils/MaterialSerializer.cpp`
  - `lib/src/bg2e/utils/MaterialModifier.cpp`
  - `lib/src/bg2e/db/scene_gltf.cpp`
  - `lib/src/bg2e/ui/DrawMaterialEditor.cpp`
  - `apps/bg2e_composer/src/command/material/MaterialInteraction.cpp`
  - `lib/include/bg2e/draw/ShaderData.hpp`
  - `lib/src/bg2e/draw/ShaderData.cpp`
  - `lib/include/bg2e/draw/pipeline/GBuffer.hpp`
  - `lib/src/bg2e/draw/pipeline/GBuffer.cpp`
  - `lib/include/bg2e/draw/pipeline/TemporalSurfacePass.hpp`
  - `lib/src/bg2e/draw/pipeline/TemporalSurfacePass.cpp`
  - `lib/src/bg2e/manipulation/DrawPicking.cpp`
  - `lib/include/bg2e/utils/ViewpointAnalyzer.hpp`
  - `lib/src/bg2e/utils/ViewpointAnalyzer.cpp`
  - `lib/src/bg2e/draw/CollectRayTracingInstancesVisitor.cpp`
  - `lib/include/bg2e/draw/pipeline/SoftShadowPass.hpp`
  - `lib/src/bg2e/draw/pipeline/SoftShadowPass.cpp`
  - `lib/src/bg2e/draw/pipeline/TracingPersistentResources.hpp`
  - `lib/src/bg2e/draw/pipeline/TracingSceneResources.cpp`
  - `shaders/src/draw/raster/gbuffer.frag.glsl`
  - `shaders/src/draw/raster/gbuffer.frag.metal`
  - `shaders/src/draw/temporal_surface/surface_v2.frag.glsl`
  - `shaders/src/draw/temporal_surface/surface_v2.frag.metal`
  - `shaders/src/draw/lib/tracing.glsl`
  - `shaders/src/draw/lib/tracing.metal`
  - `shaders/src/draw/lib/rtgi_any.glsl`
  - `shaders/src/draw/lib/rtgi.metal`
  - `shaders/src/draw/lib/rt_reflections.glsl`
  - `shaders/src/draw/lib/rt_reflections.metal`
  - `shaders/src/draw/query/soft_shadows.comp.glsl`
  - `shaders/src/draw/query/soft_shadows.comp.metal`
  - `plans/text-components/summary.md`
  - `plans/text-components/progress.md`
- Delivered behavior and API decisions:
  - `MaterialAttributes::doubleSided()` / `setDoubleSided(bool)`, default false; assignment uses authored revisions. JSON uses `doubleSided`; glTF imports its equivalent flag. The existing full attribute-copy clone/snapshot/replacement paths inherit the value; Composer's separate interactive value capture/write/equality now includes it for undo/redo.
  - Composer's material editor exposes **Double sided** beside Transparent, Solid and Unlit. The existing patch/command owner applies it to live materials and definitions.
  - Packed material flag 128 participates in classification revisions. GBuffer and temporal receiver replay each own shared Back/None cull variants for writable/read-only depth. Selection uses a shared variant per pick invocation, without changing tool/gizmo pipelines or alpha-independent rectangle selection. Viewpoint analysis also classifies each submesh by sidedness.
  - Tangent-frame policy: retain the authored front-face TBN, apply normal mapping, then negate the whole resulting normal exactly once on a double-sided backface. Temporal normals use the same policy before previous-frame transport. Reflection shading uses geometric winding, not the perturbed normal, to decide reversal.
  - Tracing candidates reject backfaces for single-sided materials before alpha testing; transformed triangle edges preserve winding under mirrored transforms. Single-sided instances are nonopaque to traversal so query/any-hit rejection can run. Double-sided non-cutout instances retain the opaque shortcut. Soft-shadow metadata now includes the model matrix (112-byte CPU/GLSL/Metal stride); its persistent identity includes transforms to avoid stale shadow tables.
  - Scope compatibility note: raster/picking retain Back culling by default. Traced occlusion/reflections and viewpoint analysis previously accepted both sides unconditionally; these paths now follow the material. Existing scenes relying on invisible backfaces as ray blockers can therefore change even with the default false. Enable Double sided on such thin surfaces. This is an explicit consistency correction, not an API migration.
  - No changes to `gpu`, fullscreen/overlay semantics, CMake, dependencies, assets, text classes or tutorial sources.
- Source review performed (not a build): inspected material packing/cache revisions, attribute cloning/snapshots and Composer commands, BG2 JSON persistence and glTF import, raster/temporal/picking/view analysis, shadow table persistence, GLSL query/any-hit and Metal tracing consumers. `git diff --check` passed. No configure/build/application/test/runtime checks were performed or added. `skills.md` was not present.
- Composer manual checks to communicate to the user:
  1. Build both library and Composer (including changed shaders).
  2. Select an existing plane/thin mesh, open its material editor and toggle Double sided. Inspect both sides with lighting, with and without a normal map; also inspect a mirrored/nonuniformly scaled instance.
  3. Select from the back when enabled; verify backface selection remains unavailable when disabled. Alpha cutouts must not turn picking into per-pixel alpha selection.
  4. Save/reopen, copy/paste or replace materials, clone geometry and exercise undo/redo; confirm the value and live appearance survive.
  5. Where supported/enabled, inspect soft shadows, AO/GI and reflections from both sides, with an alpha-cutout material and mixed single/double-sided geometry. Check temporal accumulation after a live toggle for stale classification artifacts.
- What cannot yet be checked: no font registration, text rasterization/component, locked generated albedo or text authoring exists. Vulkan and Metal changes have source-level parity only; unsupported ray-tracing devices retain existing feature unavailability.
- Known limits and unfinished work: user compilation/runtime validation pending. UV-space bake receiver capture intentionally retains authored normals and no culling: UV winding is not a viewing side, and the existing lightmap has one value per texel, not separate front/back lighting. Existing tracing texture-capacity neutral/scalar-alpha fallbacks remain; no expanded capacity or transparency redesign. Generic picking remains alpha-independent. Diagnostic `rt_probe` shaders use their own synthetic probe material schema, not scene materials, and are unchanged.
- User-reported result and acceptance evidence: none yet. The implementation request authorized step 1, not acceptance or step 2.
- Next milestone and its purpose: step 2 adds independent texture source-edit/export metadata, protects only source replacement/removal in editor-owned mutation paths (including material paste), and retains serializable UV/sampling controls while omitting ignored sources/assets.
- Exact next action / file to resume: await user feedback; fix build/behavior reports in the affected files above. On acceptance/continuation, reread this progress file and `step-02-texture-metadata.md`, then start with `lib/include/bg2e/base/Texture.hpp`.
