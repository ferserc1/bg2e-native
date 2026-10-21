# M10-02 — Audit final API reference and extension contracts

[Milestone summary](summary.md) · [Plan index](../index.md) · [Documentation policy](../documentation-policy.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see progress. **Prerequisites:** M10-01.

## Scope and deliverable

Audit existing maintained `doc/` pages and M01–M09 documentation handoffs against final source, products and distributed coverage. Record findings and localized corrections for the final M10 review. This step does not postpone earlier documentation authoring to M10.

## Audit work

Check [M07 reviewed contracts](../milestone_07_bg2e_composer/review.md) against
final signatures: detached root transfer, subclass/exact clone/custom resource
hooks, parsed CPU DB data versus stopped GPU realization, distinct request/document/
renderer identities, helper exclusion and conditional bool SelectableList scopes.
Distinguish full resource save from temporary material adoption and standalone
protected-batch publication; do not retain deleted overloads as usage recipes.

1. Audit existing API indexes/reference and lifecycle/ownership against final headers for draw/pipeline/environment, GPU/reflection/scene/bake/utils/manipulation/UI/DB/app.
2. Check accessor and reflected settings catalogs, identifiers/schema/precedence, import/export and runtime rebinding; verify clone ownership, safe rare edits, graphics-free versus complete IO and public component injection.
3. Check explicit backend bindings/native boundaries, primitive versus SDK/runtime capabilities and genuine Vulkan/Metal differences. Confirm no unsupported ResourceManager, Metal upscaler, DLSS, forward/MSAA or compatibility promise.
4. Correct localized inconsistencies in existing pages. Missing substantive reference belongs to its M01–M09 documentation owner; do not invent unimplemented APIs.

Include the [M02 review](../milestone_02_manipulation/review.md) in the source
map: distinguish transient selection/picking IDs from queue/TLAS identities,
document synchronous RGBA8/D32 operations and stopped-boundary requirements,
separate stored CPU material definitions from live attributes, and describe
preview/overlay/gizmo ownership and cleanup. Preserve the documented M02 evidence
limit: project-lead continuation is acceptance under protocol, with later
backend/scenario coverage unspecified.

## Verification and exit

Existing API documentation agrees with final declarations, metadata, capabilities and public extension points.

Perform relevant static checks. Compilation/runtime validation belongs to the project lead unless explicitly requested. Deliver one step, update progress and wait for lead acceptance under the execution protocol. No new engine feature, build-system change or future API documentation is authorized by this audit.

## M03-10 reviewed implementation requirement

Audit SurfaceInputs formats, snapshot capacity/lifetime, signal normalization/confidence, requested settings versus availability and example-owned transforms against final source. Distinguish bounded display histories from exact bake/offscreen accumulation and final SMAA from denoise; preserve retirement-era renames without retaining deleted APIs.

Source and validation boundaries: [M03 technical review](../milestone_03_ray_tracing/review.md). Include these conditions in this step's implementation and exit checks.


## M04-07 reviewed implementation requirement

Audit M04's actual contracts: one live integrated target/open frame, exact valid counts, AO/GI normalization and zero-reference policy, strict half-float raw-GI saturation rejection, nearest-seed dilation, RGB32F versus display-half limits, explicit indirectTextureMode, CPU UV preview and UI-session resource ownership. Standalone/batch/export APIs are implemented; use the final M09-07 owner and lifetime contracts.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).
## Later tracing capacity and shader ABI audit

Audit [the M07 16384 policy](../milestone_07_bg2e_composer/tracing-capacity-16384.md):
exact device/reservation-clamped counts, ShaderConstant/ShaderLib specialization,
Metal argument-record groups, current native effect shader interfaces v3, and explicit
neutral-texture overflow. Persisted reflection/settings IDs remain v1 where their
schema is unchanged. Old 32/128/256 effect ceilings belong only to historical
records or deliberately small primitive probes, not maintained product guidance.

## M07-08 source reconciliation

At HEAD `78943d5820e69363c942d71136f479a0df87128e`, current source supersedes the
historical v2 table handoff: AO/GI/reflection use native v3 with Vulkan scene set1,
persistent material arrays set2 (bindings5/6), specialization ID0 and unchanged
Metal record buffers8/9. Direct query shadows use `draw.query.softShadows.v5`,
Vulkan invocation set0, scene set1 and persistent alpha set2 (bindings16/24),
Metal records5/6 and reserves (2,1,10). AO/GI/reflection reserves are respectively
(4,2,13), (5,3,15), (6,3,16). Exact device-clamped capacity up to 16384, neutral
pair overflow and object-local ownership remain. Consume current maintained
[tracing reference](../../../../doc/api/draw/TracingSceneResources.md) and source,
not historical ABI labels. This is static reconciliation, not a new native test.

## Current engine preservation requirements — 2026-10-21

Audit all R01–R14 against final source, particularly effective/authored alpha,
revision/native generation keys, persistent v3/v5 native binding sets, exact
RecordingCompletion rather than frame numbers, pool leases and limits, lazy
fingerprint versus accepted history, typed-only notification safety and public
CpuProfile/GpuProfile/SubmissionTiming/Performance APIs. Distinguish Vulkan timestamp
boundaries from Metal encoder envelopes. Public pass injection does not require
exposing private concrete cache/ABI helpers. Verify bool noexcept writer signatures
and false handling rather than documenting thrown export exceptions.

Read the [source reconciliation](../code-reconciliation-2026-10-21.md); current engine code overrides historical implementation assumptions. No native/performance result is implied by this source baseline.

## M08-06 standalone API facts to reconcile

Audit actual standalone context/baker/assembler/batch/writer declarations and final
post-M09 accessors/umbrella exports. Preserve one live target, source/target/device
identities, exact completed UV samples, public pass injection, immutable/completed
reuse, staged complete-generation output, typed native HDR and bool-checked cloned
model snapshots. Generic db file/archive/parsed data APIs survive independently
of the retired specialized production writer. Do not document tentative SceneAssembler
or Batch aliases, arbitrary camera-profile import, automatic custom resource
inference or a general ResourceManager as implemented final APIs.

## M09-04 actual native interoperability contracts

Use [the native retirement delivery](../milestone_09_cleanup/native-retirement.md)
and doc/api/gpu/Gui_bridges.md as current evidence. Public headers now include
gpu/GuiRenderer.hpp, gpu/StandaloneGuiWindow.hpp and gpu/WindowGraphics.hpp,
exported by gpu/all.hpp. Record portable primitive borrowing, explicit shutdown
before context/device destruction, Vulkan/Metal texture-generation and encoder
ownership, standalone native RAII and drawable-pixel semantics. The old UI
backend source paths are adapters; native implementations live exclusively in GPU.
Do not reproduce retired render wrappers or expose ImGui types as public APIs.

## M09-06 declaration/umbrella audit input

Use public-api-evidence.tsv and umbrella-closure.tsv beside
[final-product-validation.md](../milestone_09_cleanup/final-product-validation.md).
They cover all public headers and eleven portable entry roots; lexical type/alias
spelling and BG2E_API marker text require reading actual namespace/signature and
ownership before reference authoring. Explicit/backend headers outside umbrellas
remain supported at their documented native seams. Binary exports have not been
inspected. Preserve actual DrawOffscreenApplicationConfig/Delegate names and
canonical shared accessors rather than introducing planned-but-absent names.

## Final M09-07 source baseline and handoff

Use [the final review](../milestone_09_cleanup/review.md) and its
[page inventory](../milestone_09_cleanup/documentation-handoff.tsv). M09-06 and
its scene/Metal/VMA corrections are accepted by continuation. Production is
`bg2e::draw`; GPU is its low-level backend abstraction. Retired render APIs and
old application targets have no compatibility promise. The source catalog lists
33 products; native scenario/backend/performance coverage remains unspecified.
M09-08 authors the complete cumulative maintained documentation before M10.
M10 audits that delivered documentation and makes localized corrections; do not
defer missing API, tutorial, build or application authoring to this milestone.

Read actual draw/Renderer.hpp, Upscaler.hpp, SettingsCatalog.hpp, SettingsCodec.hpp and RenderSettingsPreferences.hpp; bake/BakerContext.hpp and standalone owner headers; scene/DrawableComponent.hpp and ComponentFactoryRegistry.hpp; db/scene.hpp; GPU native GUI seams. Check exclusive renderer ownership, retained shared component owners, generation-safe settings, deep clones, live material aliases, borrowed bake devices and one live target. Concrete standalone APIs are implemented: StandaloneBakerContext, StandaloneLightmapBaker, StandaloneBakeSceneAssembler, StandaloneBakeBatch and LightmapOutputWriter.
