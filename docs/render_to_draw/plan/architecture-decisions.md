# Operative architecture decisions

[Plan index](index.md) · [Review protocol](review-protocol.md)

These decisions consolidate the user's conclusions. They supersede conflicting earlier assessment suggestions. Names proposed for new extension APIs may be refined during implementation, with pending steps updated at each milestone review.

## 1. Namespace ownership

| Namespace | Responsibilities |
|---|---|
| `gpu` | Low-level graphics/device capabilities, buffers/images/subresources, native Vulkan/Metal implementation, shader binding data, command execution and necessary SDK/native bridges. |
| `draw` | Engine, Drawable/material/texture resources, runtime Renderer/delegate, settings facade, draw visitors/queues/bindings and generic upscaler interface. |
| `draw::pipeline` | **Public**, advanced renderer construction objects: RenderLayer, GBuffer/pass objects, shadow/indirect/reflection passes, temporal/denoise, motion vectors and final processing. Not a private/internal namespace. |
| `draw::environment` | Public cubemap/sphere conversion, irradiance/specular prefilter, skybox and BRDF generation/resources. More directly useful standalone than pipeline internals. |
| `bake` | All baking map resources/configuration/pixels, contexts, executors, UV-surface/composition/accumulation, UV preview, standalone assembly/batch and specialized output/preflight orchestration. May depend on draw/pipeline/gpu/scene/DB. |
| `geo` | UV2 atlas modifier and geometry operations remain here. |
| `utils` | New generic BlueNoise and experimental viewpoint analysis/candidate helpers; existing TextureCache retained with draw overloads. |
| `manipulation` | Editor selection, input capture, gizmo/manipulation objects and rendering integration; scene/draw/GPU dependencies are acceptable. |
| `scene` | Graph/components, CPU descriptions, graph update/input/transform visitors, dual DrawableComponent during transition. No new data-binding implementation here. |
| `db` | Low-level file/representation serialization and high-level engine-aware resource loading/saving. |
| `ui` | Wrapper widgets/inspectors/settings/texture displays, adapted by complexity; no ImGui public types. |

`draw::pipeline` is the proposed name for the advanced-public subnamespace. Its classes must be usable by external renderers and bake. Do not hide them behind PImpl-only/private classes by default. Existing necessary private application execution classes may remain; new private classes require a concrete rationale, not a generic preference for hiding implementation.

## 2. Reimplementation and coexistence

Production render remains readable and operational until M09 primarily as a behavior reference. New logic uses gpu. Shared engine-dependent objects use overloads and an initialized execution-mode tag or equivalent simple state; wrong-mode access is explicit, never an implicit second engine initialization. Eliminate engine dependencies only in functions useful without graphics initialization or where removal is simple.

New Drawable lives in draw and retains its connector role: CPU mesh, submesh attributes, GPU mesh/material resources and tracing geometry as needed. Do not implement the external-realization redesign suggested in the original analysis. DrawableComponent supports `scene::Drawable` and `draw::Drawable` through typed construction/setter/accessor paths. Keep its original accessors for production and add clearly named experimental access while both types coexist; never silently downcast one to the other.

In the new `draw` API, name backend-realized resources with the `draw` prefix (for example, `DrawMesh`, `drawMesh()` and `drawMaterial()`), including aliases and internal fields where applicable. Existing `renderMesh()`/`renderMaterial()` names remain part of the legacy production API until its planned removal.

A Drawable clone duplicates CPU geometry **and GPU mesh/material realizations**, including associated acceleration resources when built. Preserve independence of mutable material/texture GPU objects as required by this contract; CPU source data sharing must not undermine editable clones. TextureCache may share ordinary explicitly cached immutable inputs, but clone must not route through shared mutable GPU realization paths. No default instancing; dedicated instancing components are future work.

Mesh/texture edits are uncommon: synchronize producers, `waitIdle`, update/rebuild and resume. Do not use frame-resource rings for those objects. Mutable per-frame shader data and effect outputs may still need slot-safe resources; temporal history is a separate rendered-frame sequence. WaitIdle is a safe edit boundary, not routine per-frame rendering.

TextureCache remains a singleton/service with typed production/draw overloads and minimal separate backing storage/mode validation. Avoid redesign into ResourceManager, generalized asset catalogs or a new persistent resource-identity framework. Prevent accidental cross-device resource reuse with the smallest necessary context ownership check/clear policy; do not expand the cache project.

## 3. Reflection-driven settings without duplicated values

`draw::RenderSettingsPreferences` keeps no configuration values. It exposes getters/setters and property enumeration over active renderer/pass/upscaler objects, plus save/restore and JSON serialization/deserialization. Holding active target references, metadata bindings, a catalog generation and a persistence service is acceptable; copying renderer values into a parallel preferences model is not.

Reuse reflection PropertyInfo/TypeInfo/TypeRegistry and their accessor/type/metadata contracts. Reflection currently describes values and UI but is not itself a serializer: implement an explicit settings codec using getters/setters. Enums are erased as `int64_t`; validate types/ranges/options and do not convert display option indices into configuration identities. Runtime visibility is an **instance-specific filtered view**, not mutation of shared static type metadata.

UI-facing objects register/activate their visible settings. Ordinary C++ accessors remain complete, even for options not shown in UI. A runtime renderer or pass/upscaler replacement detaches old property targets, updates generation, binds new objects and refreshes the catalog. The renderer objects remain the source of truth. No requirement to store disconnected profiles inside the stateless facade; persisted documents are explicit data artifacts.

Export contains schema version, stable renderer/upscaler identity and stable property keys/types/values. Import validates first, reports unknown/unavailable fields, and applies setters at a safe update boundary; malformed input is not silently coerced. Persisted defaults and explicit imported documents have a declared precedence. Headless code uses the same codec without initializing UI.

M01-12 delivers this through SettingsCatalog/SettingsCodec/SettingsImport and the optional facade; see [the actual contract](milestone_01_deferred_renderer/reflected-settings.md). Catalog generations are unique across instances, renderer callbacks retain weak ownership, and queued edits commit before acquisition after GPU drain. Settings wire numbers use typed decimal strings to preserve precision through JsonNode; Preferences stores an escaped-safe hex document. Explicit documents bypass persisted defaults and omitted fields retain live values.

## 4. Runtime delegate and extensible public components

Introduce a non-template scene-renderer delegate owning a polymorphic `draw::Renderer` through explicit ownership. It can replace the active renderer at a safe frame boundary while retaining the scene. Only RendererDeferred is supplied. A test can replace it with another instance/configuration of the same class to verify lifetime/settings rebinding without inventing a second renderer product.

Public configuration structs select compatible layer, illumination, shadow, temporal/denoise and final-processing objects and shaders. Define borrowed/shared/owned lifetimes and format/extent/input requirements. Defaults create normal engine objects; applications can subclass and replace them. Reuse this pipeline in bake rather than exporting a second independent algorithm family.

No MSAA fields/targets/promises in the new renderer architecture. All planned renderer paths are deferred and single sample. Production-only MSAA survives only until its legacy deletion.

## 5. Hardware capabilities and backend specificity

GPU exposes device primitive capability/format/usage/limit queries. Draw combines those facts with SDK availability, successful creation and selected component requirements to determine effective features and UI options. FSR availability is not solely a hardware boolean; neither is a valid shader path implied by an acceleration capability.

Shader binding descriptions retain explicit Vulkan and Metal indices; extend resource arrays, offsets/ranges, stage visibility and read/write primitives as needed. No new shader abstraction, automatic shader translation, backend-neutral binding language or extra shader authoring rules. ShaderLib remains a naming/loading convenience; Vulkan and Metal shader implementations may differ substantially.

All **new native Vulkan implementation** stays inside gpu, including a narrowly scoped native FidelityFX bridge if required. Draw's FSR policy object can request its GPU backend bridge; dynamic casts to `gpu::vk` implementations are legitimate at that integration seam, with explicit device checks and native handlers added where necessary. The bridge handles SDK/native commands, not scene/material/renderer policy. UI wrappers similarly delegate native registration/drawing to a GPU backend integration without introducing a GPU dependency on `ui`. This reconciles native SDK/UI interop with the requirement to remove Vulkan implementation outside GPU. Existing production/native integration code is retained as reference during coexistence and removed or relocated in M09; new code must not expand it outside GPU.

For complex operations such as readPixels or mipmap generation, select generic command primitives versus backend-implemented convenience operations by **comparative implementation complexity**. A documented native convenience method in GPU is acceptable when a higher-level implementation would require more awkward infrastructure. Preserve precise state/completion/readback format semantics.

## 6. Upscaling and temporal scope

Public `draw::Upscaler` (proposed name) has stable identifier/label, capability/options enumeration, ordinary accessors, reflected UI parameters, input/output extent contract and recording/reset lifecycle. Applications can register/inject external implementations. Final output does not hard-code an FSR enum as the universe of modes.

Default output is user-selected internal render-resolution reduction followed by ordinary fullscreen resampling to output, with no temporal engine processing. Preserve lower resolution when FSR is unavailable; do not force 100%. FSR is optional in M01 on supported Vulkan/SDK paths. Metal native upscaling and application-integrated DLSS are future examples of the extension mechanism, not engine implementation tasks here.

FSR intrinsically maintains temporal SDK state and requires jitter/motion inputs. M01 permits its own SDK state and minimal camera-depth motion-vector generation, but **does not implement engine TemporalAccumulator or DenoiseFilter**. M03 introduces reusable engine histories/denoise and revisits moving-object motion/reset needs. The non-FSR default is fully non-temporal in M01.

## 7. DB levels and graphics-free scene representation

Project lead correction during M07: unregistered component types in scene JSON
must never abort loading. Schema and draw factories warn and return null; graph
load skips them and continues. Registered component parser/resource errors remain
separate failures. Raw document IO preserves unknown JSON; typed saves do not
retain skipped components. This supersedes earlier strict unknown-type rejection
described in historical M01/M06/M07 handoffs and probes.

Low-level serialization/deserialization reads/writes schema, graph descriptions, resource references and supplied CPU data. It does not fully load images/meshes into GPU resources or claim to export GPU-only textures. A graphics-free scene representation can be unresolved; complete resource readiness is not promised.

High-level load/save overloads accept the chosen Engine, invoke low-level IO, realize resources, or read device images into host buffers for export. CPU mesh file parsing can still operate without an engine; low-level serialization must not initiate unrequested resource loading. Define high-level output completeness and progress/failure semantics rather than silently discard a GPU-only generated texture.

## 8. Milestone scope and deferred decisions

M01 includes raster PBR/layers/transparency/IBL, alpha-aware query soft shadows, reflection-based settings and FSR/scale output. No baking, headless application, editor manipulation/apps, engine temporal accumulation or denoise. Its original production reference uses opaque/transparent models, a floor, camera rotation, environment, materials and lights; retain these supported parts and remove unsupported RT debug/effect controls.

Advanced GI/AO/reflection implementation choices are deliberately left to M03. Re-read current GPU RT documentation then. Vulkan uses actual RT pipeline stages/SBT; Metal uses compute/intersection shaders with equivalent results. Do not invent a true native Metal RayTracingPipeline or force shared shader source. Non-opaque geometry support for query shadows is already required in M01.

All bake resources—including LightmapSettings, LightmapPixels, UvMapPreviewRenderer, contexts, StandaloneBakeSceneAssembler and StandaloneBakeBatch—live in bake. UV2 stays in geo. New optimal-viewpoint tools live in utils; existing `render::ViewpointAnalyzer` (the actual current class name) stays untouched until legacy removal. Manipulation remains a high-level engine namespace.

Specialized LightmapOutputWriter preflight/batch/model-copy policy also belongs in bake; generic image/scene file encoding remains in DB. Preserve the existing production DB entry points during coexistence, then remove those wrappers in M09.

Production's generic image-processing/fullscreen/attachment helpers must remain possible through public draw::pipeline resource/pass objects and GPU primitives. SMAA is spatial AA, not MSAA: its implemented facility is preserved in M03 as a public optional processor/final-output component, outside the default resolution-reduction path. It does not justify adding MSAA or a forward renderer.

## 9. Shader organization

Proposed source layout: `shaders/src/draw/<pipeline>/` with the GLSL and Metal stage files together, and `shaders/src/draw/lib/` for shared `.glsl`/`.metal` includes. Bake shaders use `shaders/src/bake/<pipeline>/` with the same paired layout and may reuse public draw kernels/objects. Compiled libraries retain distinct `draw/<pipeline>` and `bake/<pipeline>` paths, so old production shaders remain unambiguous through M08.

Use actual ShaderLib resolution/entry points and current CMake helper behavior as evidence; header comments/historical docs are not sufficient. Do not flatten libraries into one output folder where stage filenames collide. Complete installation, app bundles and standalone packages, not only development-tree loading.

## M01-13 implemented resolution/upscaling boundary

See [upscaling.md](milestone_01_deferred_renderer/upscaling.md). Open Upscaler injection is through shared RendererConfiguration ownership; implementation-owned live reflection metadata is discovered without an enum. ResolutionSettings enabled/scale is the authoritative internal/output ratio even without FSR. Existing native/fixed behavior stays the startup default until reduction is enabled. RendererDeferred reconstructs output-sized linear HDR before the single final tone/color operation; final replacements use finalContract(output, depth, true). FSR native handlers and SDK are confined to gpu::vk::FidelityFXBridge, with separate SDK/backend/hardware status, SDK-owned histories and camera-only motion. No general engine history/denoise is introduced.

## M01 review: reusable baseline and deferred boundaries

The [M01 review](milestone_01_deferred_renderer/review.md) is the implemented-source baseline, not a new architecture approval. Public rendering/subresource primitives, synchronous tight raw color readback, generic query opacity, live settings/codec, open upscaling and environment snapshots are reusable. Depth readback, general 2D arrays, draw-aware Selectable/schema hooks, public overlay/effect surface access, arbitrary RT hit-material lookup and windowless draw initialization remain their explicit M02/M03/M05 tasks. Query IDs/table order, selection IDs, node IDs and catalog/renderer/job generations are distinct identities.

Keep deep CPU/GPU clones and borrowed Engine/device lifetimes, phase-specific production coexistence through M08 and removal at M09. M01 private graph images do not authorize copying a private renderer into tools: add minimal public borrowed profiles where later passes require them. The small eight-pair UV/alpha shadow table is not a settled advanced RT architecture; M03-01 retains the project-lead-owned checkpoint. FSR temporal state is not engine history, exact scene averaging or bake accumulation. No ResourceManager redesign, native Metal upscaler or DLSS implementation is introduced by this review.


## Incremental documentation closure (project lead direction, 2026-10-15)

M01–M09 each add a final documentation step after their technical review, updating canonical maintained doc/ to the implementation delivered so far. Do not document unimplemented later milestones as available. M10 remains as a lighter final audit of the cumulative documentation, retaining its stable step IDs and original final coverage. [Documentation policy](documentation-policy.md) maps every former M10 authoring requirement to its milestone owner. This scheduling change does not authorize implementing documentation steps in the planning-update turn.

## M03-01 proposed advanced tracing contract — pending acceptance

The [checkpoint](milestone_03_ray_tracing/rt-design.md) recommends Vulkan native raygen/miss/closest-hit/any-hit and Metal compute/intersection queries, packed indexed geometry, bounded native image/sampler arrays and explicit tracing/history invalidation. It compares alternatives, capabilities, profile capacities, cutout behavior and deferred blended secondary transport. These are proposed decisions; they become operative only on project-lead acceptance/continuation of M03-01. Existing eight-pair shadows, public headers and production rendering are unchanged.

## M03-01 acceptance and M03-02 implementation

The lead's next-step instruction accepts the [M03-01 recommendation](milestone_03_ray_tracing/rt-design.md); its earlier pending status above is historical. [M03-02](milestone_03_ray_tracing/rt-gpu-data.md) delivers AnyHit/native SBT hardening, additional Vulkan visibility and full fixed sampled-resource arrays with explicit Metal argument-buffer indices. Logical-device capacities and matching 32/128/256 shader profiles are separate from RT flags. GPU remains policy-free; public draw snapshot/material construction is M03-04. Metal array support is currently compute/RT color 2D only. No CMake change or build/runtime execution accompanies this delivery.

## M03-03 noise layout implementation

utils::BlueNoiseData/sampleNoise2D expose the existing ranked bytes without graphics initialization; utils::BlueNoise uses a borrowed gpu::Device and shared image/sampler ownership. Logical layers are stacked in a single color 2D RGBA8 atlas with exact byte expansion and paired native tile-local sampling helpers. This avoids introducing general image-array primitives solely for the four immutable tiles. Disabled/missing noise selects an explicit deterministic hash path. Effects/bake own rendered-sample sequencing and reserve resource bindings explicitly. The existing private byte header is shared from its historical render source path until M09 relocation; no production API is changed. See [blue-noise.md](milestone_03_ray_tracing/blue-noise.md).


## M04 reviewed implementation baseline (2026-10-18)

Integrated bake lives in bake and reuses public draw AO/GI world-surface profiles.
One live target and one open frame per context serialize sample completion.
Separate public raw-sum/count, composition and nearest-seed dilation owners
publish RGBA32F multipliers only after completed-work validation. Byte GI CPU
conversion applies exposure/clamp; float output does not. Raw half-float GI
saturation at 65504 is rejected conservatively. The windowed example confirms
completion through one synchronous immediateSubmit per update; headless context
ownership and exact camera-frame averaging remain M05/M08 work.

Explicit MaterialAttributes::IndirectTextureMode reuses the AO texture slot for
scalar AO or baked RGB AO/GI on UV2; simultaneous authored AO plus baked-map
textures are outside this six-slot representation. Baked GI preserves specular
IBL/reflection contributions, direct light and emission. The RGBA16F G-buffer
multiplier has a display range separate from the full-float bake result. CPU
UV-map diagnostic rasterization plus generic sampled-image upload replaces the
proposed GPU geometry preview implementation; it is not a native API wrapper.
Both refinements are recorded as intentional contract changes accepted by lead
continuation, with their limitations visible in the [review](milestone_04_lightmaps/review.md).

Paired draw and bake libraries now have distinct compile/intermediate trees,
shared aggregate dependencies, incremental bundle refresh and install coverage.
GPU-only live texture ownership/JSON mode metadata do not constitute export;
complete image materialization and standalone batch policy remain future owners.

## Project lead default-renderer policy (2026-10-19)

With no explicit `draw::RendererConfiguration` override, the deferred draw
renderer selects `GlobalIllumination` and its default RTGI and reflection passes
are enabled. These are requested settings, independent of device/shader
availability; unsupported effects keep their status and use established fallback
paths. An injected pass, explicit mode/settings, or an application's intentional
comparison configuration takes precedence. Applications such as `draw_model_edit`
inherit the shared default rather than duplicating it.

## Lead amendment: recoverable tracing material table exhaustion

For the M07 Composer loading correction, the lead explicitly authorizes treating
excess tracing image/sampler pairs as absent textures, with a warning, instead of
throwing and exiting. Preserve every instance/material and authored scalar values;
only the tracing snapshot degrades, never the source/raster material. The shared
GLSL/Metal sentinel is `UINT32_MAX` in both index lanes; sampling returns neutral
white or flat normal before resource access. This supersedes earlier strict
sampled-table overflow rejection, not validation of malformed geometry/resources.
Later image/sampler-management improvements remain separate.

## Lead amendment: tracing ceiling 16384 (2026-10-20)

Use the exact minimum of 16384 and available image/sampler/aggregate device budgets
after effect reservations, without rounding to 32/128/256. The [M07 correction](milestone_07_bg2e_composer/tracing-capacity-16384.md)
records Vulkan specialization ID0 and Metal Tier-2 image/sampler record buffers8/9,
native v2 shader interfaces and the conservative native sampler-budget constraint.
This supersedes earlier fixed-profile/Metal 256-ceiling wording. Material owners
and references remain local to independently importable/exportable objects; GPU
tables are execution bindings, not a global material library. Keep neutral-texture
overflow warnings. A larger resource management redesign is explicitly deferred.
## Direct shadows capacity and deferred texture optimization

The lead reports working Model Edit shadows but missing Composer scene shadows.
SoftShadowPass retained a separate eight-pair table and whole-scene neutral
fallback, outside the earlier AO/GI/reflection expansion. Extend that pass to the
same exact min(16384, available device budgets) policy with reserves (2,1,9).
Use v2 query kernels, Vulkan specialization ID0 and Metal image/sampler argument
records5/6. Table overflow must retain all instances and other shadows; only the
excess texture sample becomes neutral alpha with a warning and scalar alpha/cutoff
preserved. Do not reduce texture counts, alter alphaCutoff/material defaults,
canonicalize object resources or globalize material ownership in this correction.
The lead explicitly defers shader/resource-count improvements until the API
migration is complete; that later refactor needs separate authorization.
## Composer explicit environment selection

The M07 lead-directed [environment correction](milestone_07_bg2e_composer/environment-synchronization.md)
uses one explicit active Environment selection in Scene; nullptr means renderer
defaults. Composer has no hidden stage component. Opening a document selects its
first enabled authored Environment, later additions cannot replace a valid active
source, and removal restores default sky without promoting ignored components.
Renderer generation, queue snapshots and settings honor the same source. Retain
procedural RGB while HDR is assigned, restore RGB when cleared, and expose failures
without silently rolling back user fields. Existing non-opt-in Scene behavior and
Model Edit coexistence remain intact; no new global material/resource policy.
