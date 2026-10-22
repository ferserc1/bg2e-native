# M10-02 — Final API reference and extension audit

[Progress](../progress.md) · [Inventory](inventory.md) · [Next: tutorials](step_03_tutorials.md)

## Baseline and outcome

Delivered 2026-10-22 at HEAD `e2a32164a781dbf420a4464b25ddce9feb862c6a`,
with the complete uncommitted M10-01 delivery retained. Lead continuation accepts
M10-01, including its two writer-description corrections. Backend/scenario details
remain unspecified. This step changes maintained documentation and audit handoffs
only; no engine feature, source formatting, shader, CMake or product-script change.

The API audit compares final public declarations and relevant implementations with
maintained reference/index pages, current settings metadata, native contracts and
the M02/M03/M04/M07 review requirements. Localized discrepancies are corrected;
no substantive missing reference or new API implementation was identified within
the requested contract families. Per-example construction, CLI/build/distribution
and whole-document snippet/diagram validation remain separate M10-03–05 steps.

[api-contracts.tsv](api-contracts.tsv) records each reviewed contract, its maintained
pages, actual source anchors, conclusion and evidence limit. This is a contract
review, not an exhaustive C++ symbol parser or an assertion that every possible
signature fragment has been compiled. The inventory of all 317 headers remains
available for the final cumulative audit.

## Corrections to maintained guidance

| Finding | Correction | Source authority |
|---|---|---|
| Draw reference still advertised a supported, nondeprecated predecessor; GI/reflection/filter pages described coexistence. | Canonical production draw status and retired API boundary; no compatibility promise. | Root/draw umbrellas, Renderer and public pipeline headers. |
| DB index and complete IO page described a surviving specialized writer and glTF production overload. | Removed writer is unavailable; describe the current shared CPU parser and explicit draw realization. Single-document and whole-batch publication remain distinct. | db/scene.hpp, scene_gltf.hpp, draw_resource_io.cpp and bake/LightmapOutputWriter.hpp. |
| FrameContext table referred to retired millisecond callbacks. | Both current delta fields use seconds; retain distinct slot/presentation/scene sequences. | draw/FrameContext.hpp. |
| Preview guide advertised removed ui::UvMapPreview. | Use bake::UvMapPreviewRenderer with ui::BakeLightmapPreview and retained UI-session ownership. | Current preview headers and UI umbrella. |
| Generic settings prose implied a fixed deferred identity and did not explain automatic persistence opt-in. | Serialize the actual bound catalog identity; specify deferred versus standalone identities, strict type/schema matching and opt-in first-attachment restore/queued persistence. | SettingsCodec.cpp, RenderSettingsPreferences.cpp and current standalone parser. |
| Resource guide did not spell out materialDefinition versus material alias and divergence limits. | Stored definitions stay distinct after load; clone captures live values and cleanup snapshots them back. | Drawable.hpp/.cpp and MaterialModifier.hpp. |
| Standalone pass prose generalized null defaults and suggested filtering compatibility broader than the executor. | Null denoise disables filtering; current raw executor rejects a supplied DenoiseFilter. | BakerContext.hpp and LightmapBakeExecutor.cpp. |
| Native GUI RAII wording could imply ownership of the borrowed ImGui context. | Distinguish owned native GL context/window/backends from caller-owned ImGui context. | StandaloneGuiWindow.hpp/.cpp. |
| Performance summary claimed a fixed 16384 capacity and refit. | Capacity is device/reservation clamped, up to 16384; unchanged completed acceleration generations can be reused, while changed instances require rebuild. No current refit path. | ResourceArrayCapabilities, TracingSceneResources and native ray-tracing APIs. |
| Tracing reference omitted effective alpha/default/import/opacity details. | Authored default zero, effective threshold 0.02, normalized glTF MASK 0.5 and opaque/cutout routing are explicit. | MaterialAttributes, ShaderData, scene_gltf and CollectRayTracingInstancesVisitor. |
| Advanced optimization API pages were not directly listed in draw API navigation. | Link pools, invalidation, regions, update synchronization and profiling from draw index/reference. | Existing maintained references and current public/source owners. |

Renderer/Upscaler references also clarify the actual linear recording format/usage,
final-only same-slot contract, render-thread lifecycle and drained resource changes.
Viewpoint analysis explicitly identifies synchronous RGBA8/D32 operations.
[api-page-changes.tsv](api-page-changes.tsv) records step-entry and final hashes for
the 18 changed maintained pages, independently of inherited M10-01 changes.
M10-01's page hashes and audit JSON remain original evidence, not overwritten snapshots.

## Retained final contracts

- **Renderer and extensions:** unique renderer ownership, retained shared injected
  components, slot-specific null defaults and synchronized replacement remain.
  Compatible public layer/effect/upscaler overrides preserve formats, shader ABI
  and complete dynamic settings addresses. Private cache helpers need not become
  public. Native Metal reconstruction, DLSS, ResourceManager and forward/MSAA are
  not documented as delivered extension implementations.
- **Scene/reflection/DB:** detached root transfer, exact-then-subclass lookup,
  concrete independent component/projection clones and helper exclusion remain.
  Accessor reflection is optional and independent of schema IO. Unknown malformed
  component representations warn/skip; registered contract/parser/resource failures
  propagate. Raw JSON preservation, parsed CPU snapshots, synchronous stopped GPU
  realization and bool-noexcept complete saves have distinct contracts.
- **Manipulation/UI:** weak typed selection identities remain separate from pick,
  queue and TLAS identities. Picking/analysis are synchronous stopped-boundary
  operations. Material patches independently modify stored/live values and can
  partially fail. Overlays/gizmos/previews release before Engine/Device teardown.
  SelectableList items/endList require a true beginList. Composer request/document/
  renderer generations remain application-owned; no shared API conflates them.
- **Effects and native resources:** camera surfaces use encoded RGBA16 normals and
  D16/D32 depth/inverse VP; explicit surfaces use signed normals, RGBA32 positions
  and R8 coverage. AO visibility, GI incoming radiance/pi plus validity, and
  unpremultiplied reflections plus confidence are distinct. Display histories and
  denoise do not promise exact UV/camera means or chart isolation. Vulkan query
  support does not imply native RT pipeline support; Metal uses compute queries.
- **Bake and output:** Device/Engine/command lifetimes are borrowed; concrete contexts
  allow one live target, and integrated execution one open frame. Standalone accepts
  only its internally owned command through synchronous update. Frozen settings,
  source/target/native device identities and normally completed recordings govern
  reuse/publication. Exact valid counts, raw GI saturation rejection, channelwise
  zero-reference behavior, nearest-original-seed padding and explicit linear RGB
  UV2 material modes survive. RGB32F file precision does not extend display-half
  range. Custom dependencies need explicit hooks; staged complete model snapshots
  and final manifest publication are distinct from partial interactive adoption.

## R01–R14 reconciliation

| Requirement | Source/document conclusion |
|---|---|
| R01–R02 | Authored/effective alpha, normalized glTF MASK and ForceOpaque/ForceNonOpaque distinction preserved; blended secondary exclusions remain. |
| R03 | Material/texture/native binding revisions and no-op realization reuse remain; resource replacement still coordinates users. |
| R04–R06 | Indexed first-encounter material identities, immutable generations and neutral pair overflow remain. Exact capacity/reserves and native v3/v5 interfaces are documented, not historical fixed profiles. |
| R05 binding detail | AO/GI/reflection Vulkan scene set1 and images5/samplers6 in set2; Metal records8/9. Shadow invocation set0, scene set1 and alpha set2 bindings16/24; Metal records5/6. Specialization ID0; reserves AO (4,2,13), GI (5,3,15), reflections (6,3,16), shadows (2,1,10). |
| R07 | Exact RecordingCompletion, not frame age, certifies unchanged acceleration reuse; pending/aborted/unsubmitted work is not completion proof. |
| R08 | TemporaryResourcePool leases and retained consumers preserve pending work; default limits 512 MiB, 256 image entries, 256 binding entries, eight idle retirement visits. |
| R09 | Lazy fingerprints and retained component signatures remain separate from accepted history; unobserved edits still require explicit invalidation. |
| R10 | Conservative raster/composition regions retain full custom/debug/unsafe fallbacks; effect domains are not all region-limited. |
| R11 | Only proven typed final-color batches with suitable notification contracts may avoid drains. Generic/mixed/resource/lifecycle work remains conservative; CPU producers must stop. |
| R12 | Base CPU/GPU transport, recording sessions, async native timing and Performance window ownership remain separate. Vulkan pipeline-boundary timing and Metal encoder envelopes are not interchangeable pass totals; closing a live window does not stop recording. |
| R13 | bool noexcept failure checking, parent creation, unique sidecars, exclusive reservation/copy fallback and main-file-last publication remain; no power-loss durability promise. |
| R14 | Actual integrated/standalone APIs and bake root export are documented as delivered. Historical pending owner names are not public aliases. |

The related optimization/profiling reports retain their independent native and
measurement acceptance. No current source was changed to satisfy an older plan
or to manufacture an improvement claim.

## Verification and handoff

The contract matrix and [api-source-hashes.json](api-source-hashes.json) identify
actual inspected source owners. Step-entry preservation checks cover 1,276
non-vendored implementation/build/script/test files outside doc/: all retain their
hashes. No modified C/C++/shader file requires a new cosmetic/license review.
Inherited source style/license evidence is not upgraded into a repository-wide
formatting certification.

Fresh static navigation/includes check:

```sh
python3 docs/render_to_draw/plan/milestone_09_cleanup/audit_documentation.py --output-dir /tmp/bg2e-m10-api-audit
```

[api-documentation-audit.json](api-documentation-audit.json) records 229 maintained
pages, 1,881 checked local links/anchors, 111 project include occurrences and zero
errors, retaining two illustrative MyComponent-header exceptions. Source paths,
changed-page hashes, matrix anchors, settings/default/ABI constants and whitespace
are checked statically. No snippets, CMake, shaders, C++ probes, applications,
packages, binary exports, output reloads or profiling captures were executed.

Lead review should check corrected API/ownership wording and this source mapping.
No new executable is introduced and no compilation is required for this
documentation-only delivery. M10-02 awaits acceptance; continuation selects
M10-03 tutorial audit. No substantive authoring gap is deferred from this audit;
per-example snippet assembly, CLI/package verification and global diagrams remain
with their existing dedicated audit owners.
