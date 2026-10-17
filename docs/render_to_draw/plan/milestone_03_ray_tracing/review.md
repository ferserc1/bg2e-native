# M03 technical review — advanced tracing and reusable effect contracts

[Summary](summary.md) · [Progress](../progress.md) · [Next: cumulative documentation](step_11_documentation.md)

Date: 2026-10-17. Starting and finishing HEAD:
`15f67dff9efad51c878740bc1ddbce94f880ed46`, with preserved uncommitted M03 and
preceding work. This review is itself uncommitted. A HEAD-only checkout does not
reproduce the reviewed implementation. No reset, configure/build, shader compiler,
fixture, application or packaging execution was performed by the agent.

M03-09, including missing-window/SMAA corrections and the manipulation light editor
and active GI/reflection defaults, is **accepted by project lead continuation
instruction**. Backend/scenario coverage for that acceptance is not specified.
This review remains awaiting lead acceptance. M03 is not closed until M03-11.

## Source-to-contract comparison

| Planned area | Actual source and contract | Classification / consequence |
|---|---|---|
| Backend tracing stages | [Vulkan pipeline](../../../../lib/src/bg2e/gpu/vk/RayTracingPipeline.cpp) provides optional any-hit, aligned SBT records/addresses and allocation flush. [GI](../../../../lib/src/bg2e/draw/pipeline/RTGlobalIllumination.cpp) and [reflections](../../../../lib/src/bg2e/draw/pipeline/RTReflections.cpp) use Vulkan native raygen/miss/closest/any-hit at recursion depth one; iterative shader bounces. Metal uses compute intersection queries. AO uses compute queries. | Equivalent refinement of accepted native-backend design; query-only support does not imply Vulkan GI/reflection pipeline support. No emulated native Metal pipeline. |
| Materials/geometry | [TracingSceneResources](../../../../lib/include/bg2e/draw/pipeline/TracingSceneResources.hpp) and [builder](../../../../lib/src/bg2e/draw/pipeline/TracingSceneResources.cpp): 64-byte vertices, 160-byte instances, 256-byte materials; MaterialData 192 bytes plus independent six image/sampler indices. Geometry cache retains actual DrawMesh identity; immutable invocation snapshots own table buffers, textures, samplers and TLAS. | Matches accepted packed ABI. Submesh firstIndex/indexBase and vertexBase remain separate; no raw vertex-address abstraction. New GPU mesh identity required after reload. |
| Effective indexed resources | Fixed 32/128/256 profiles selected from logical-device nonuniform-resource limits. AO reserves (4,2,13), GI (5,3,15), reflections (6,3,16) in profileCapacity. Vulkan uses nonuniformEXT; [Metal ResourceSet](../../../../lib/src/bg2e/gpu/metal/ResourceSet.cpp) encodes complete argument arrays/residency. | Equivalent bounded-table refinement. Required/available overflow is an error; unavailable hardware is a separate status. No truncation, paging, unbounded bindless or general Metal array graphics ABI. |
| Cutouts and identity | [GLSL tracing](../../../../shaders/src/draw/lib/tracing.glsl) and [Metal tracing](../../../../shaders/src/draw/lib/tracing.metal) share alpha/UV/scale acceptance. Dense TLAS ordinal addresses table; canonical object ID is correspondence only. ForceNonOpaque exposes cutout candidates; rejected hits continue behind holes. | Matches accepted boundary. Both blended transparent classes are excluded as occluders/secondary geometry, although raster receivers can have effects. No colored transmission or transparent secondary parity claim. |
| Public surfaces | [RTAmbientOcclusion](../../../../lib/include/bg2e/draw/pipeline/RTAmbientOcclusion.hpp): CameraDepth uses encoded RGBA16 normals and D16/D32 depth plus inverse VP. WorldPositionCoverage uses signed RGBA16 normals, RGBA32 world positions and separate R8 coverage. All sampled same-device images; lifetime borrowed through completion. | Explicit profile already delivered, not a future camera workaround. M04 must generate UV-space surfaces/coverage; no private renderer target access or fabricated camera. |
| AO and GI signals | AO scalar R16 visibility, neutral one; GI RGBA16 incoming diffuse radiance/pi before receiver albedo, alpha validity. GI valid black differs from unavailable/invalid zero. AO/GI own independent quality extents; renderer mode chooses IBL/AO/GI and preserves material AO/specular IBL. | Actual normalization refinement; M04 must define bake multiplier/presence separately and avoid double albedo/PI/AO. This is not complete arbitrary secondary PBR transport. |
| Reflections | RGBA16 unpremultiplied mean geometry-hit radiance, alpha acceptedHits/actualSamples; roughness gate/spread, receiver material and camera position required. Quality extent is relative to explicit output dimensions. Lighting blends specular IBL with confidence, then receiver split-sum BRDF once. | Intentional signal refinement; misses restore specular IBL. Reflected hit shading is an approximation, not recursive full material/PBR/transparent parity. M05 must retain fractional confidence. |
| Noise | [BlueNoise](../../../../lib/include/bg2e/utils/BlueNoise.hpp) has CPU rank data and shared immutable RGBA8 stacked 2D atlas; no physical texture-array assumption. Renderer builds a shared atlas only for available effects lacking injected owners. Hash remains explicit fallback. | Equivalent layout refinement. Caller owns rendered sample/view keys and lifetime; the utility does not schedule frames. |
| Temporal/motion | [TemporalAccumulator](../../../../lib/include/bg2e/draw/pipeline/TemporalAccumulator.hpp), [kernel](../../../../shaders/src/draw/lib/temporal.glsl): key(signal,view,layer), scene sequence/generation, maxFrames 1..1024, per-pixel rejection/count, arithmetic then rolling/frozen behavior. Camera reprojection; object/resource/light changes conservatively reset. Fresh history versions retained through completed slots. | Delivered display history, not exact general N-frame or bake accumulation. No previous-object vectors. Same ordered submission queue and explicit app notification remain required. |
| Spatial denoise | [DenoiseFilter](../../../../lib/include/bg2e/draw/pipeline/DenoiseFilter.hpp): visibility/diffuse/reflection profiles, camera linear-view-depth or explicit world-distance guides, roughness/confidence handling, fresh owned outputs; unavailable/radius-zero returns borrowed source. | Reusable guide-based filter. No UV chart IDs/isolation input exists; M04 must add compatible chart isolation before filtering an atlas. Never feed denoised results into raw sample averages/history. |
| Settings and edit lifecycle | [Facade](../../../../lib/src/bg2e/draw/RenderSettingsPreferences.cpp) binds actual metadata/instance addresses; requested values serialize independently of availability/status. Codec validates merged distance/bias pairs and preserves numeric types. Weak/generation-checked queued edits reset histories, including partial-failure reports. | Delivered open catalog; no copied preferences model. Direct in-place uploads, custom shader changes and same-camera teleports still require invalidateHistories. Example-owned targets require headless/app policy, not silent schema invention. |
| Working products | [Deferred example](../../../../examples/draw/10_deferred_renderer/src/main.cpp) and [manipulation](../../../../examples/draw/11_manipulation/src/main.cpp) now create/connect/bind/draw graphics windows and have reopen buttons. Manipulation includes live per-light properties/local XYZ/Euler edits, GI selected and GI/reflections enabled. FSR/SMAA disabled initially. | M03-09's initially missing visible UI was a missing requirement, corrected before this review and accepted by continuation. Manipulation defaults are example-specific; not global renderer defaults. |
| Final processing | [SMAAProcessor](../../../../lib/include/bg2e/draw/pipeline/SMAAProcessor.hpp) implements device-only process and FinalProcessor integration. Paired edge/weights/blend/copy shaders and official AreaTex/SearchTex. Only after final color, once per frame, before overlays/UI. | Preserved orthogonal SMAA 1x profile, no diagonal/corner/temporal SMAA or MSAA. Default stays ordinary resampling; independent of RT and optional FSR. Keep vendored lookup bytes/licenses on retirement. |
| Legacy shadows | [SoftShadowPass](../../../../lib/src/bg2e/draw/pipeline/SoftShadowPass.cpp) retains eight distinct cutout pairs. Specific capacity exception remains observable by table callers; pass uses neutral whole-scene visibility, sceneSupported/status and retry. | Intentional integration contract change accepted with M03-09. Advanced tables remain independent. This is graceful degradation, not arbitrary-material query-shadow parity. |

## Working-product evidence and verification limits

| Evidence | What it establishes | What it does not establish |
|---|---|---|
| Lead stated M02 manipulation worked on macOS Metal and Linux Vulkan | Explicit historical M02 backend report | No automatic M03 shader, GI, reflection or performance coverage |
| Lead reported no visual/performance difference, then supplied two example screenshots | Scene windows rendered; requested effect controls were absent. Source confirmed missing UI wiring. | No image proves enabled GI/reflection output, performance or native profile support |
| Subsequent fixes and latest continuation | M03-09 accepted under execution protocol, including light-editor addition | Exact tested hardware, combinations, numeric output and diagnostics were not specified |
| Agent source review | Public declarations, native dispatch paths/ABI, bounded arrays, candidate filtering, signal/ownership/reset contracts, actual callers and pending plan updates inspected | No agent compilation/runtime/backend parity or speed claim |
| Manual fixtures | Existing tests/gpu/advanced_tracing, tests/utils/blue_noise and tests/draw/{rtao,rtgi,rt_reflections,temporal,denoise,advanced_integration} provide lead-run surfaces | Their existence is not a passing execution record |

Agent static checks: 338 pipeline-header/implementation/draw-shader files have full
GPL text in their opening comment. Source inspection covered control-flow braces,
C++ opening-brace/namespace/include style and method groups at the changed contract
sites; automated lexical checks are not a complete C++ style/parser proof. Found
adjacent function definitions in SoftShadowPass.cpp, RenderTarget.cpp and GBuffer.cpp;
inserted blank separators only and verified non-whitespace tokens unchanged.
Focused 59-file source/include/delimiter checks and native SMAA binding/ordering
checks pass; deferred Metal samplers are checked against declarations. Local links
and git diff --check are checked separately. Existing third-party licenses retained.

## Completed, deferred and unavailable work

Completed public surfaces/effects, packed tracing tables, temporal/display filtering,
noise, live controls/preferences, two existing products and optional SMAA are source
present and accepted through M03-09. No new example loop/target was introduced here.
GPU remains independent of higher-level scene/render/db/utils APIs; production
render remains operational/reference and separate through M08.

Intentionally outside M03: UV chart generation/raster coverage, baked-map presence
and RGB application, exact bake/scene averaging, windowless application scheduling,
full edited scene IO/new applications, previous-object motion, general transparent
secondary transport, ray-cone LOD, recursive reflection transport, paging and MSAA.
Do not describe these as available or as measured production parity.

Open verification gaps: native Vulkan/Metal numerical equivalence, failure-profile
fallback, asynchronous lifetime, large cutout scenes, HDR/confidence edges and
actual GI/reflection cost. Use the [integration matrix](../../../../tests/draw/advanced_integration/README.md).
Lead continuation accepts the delivered step without supplying a new measurement
record; those gaps remain visible rather than blocking acceptance by assumption.

## Pending-plan impact and concrete edits

| Pending work | Required action in its real step files |
|---|---|
| M03-11 | Reconcile cumulative maintained API/tutorial/app pages with corrected visible windows, manipulation light poses/defaults and final processing. Preserve actual validation limits. |
| M04-01/02/03/04/06 | Use WorldPositionCoverage and matching-capacity TracingSnapshot; add UV charts/coverage and chart-isolated filtering, exact raw AO/GI averages and explicit baked presence/normalization. Disable live indirect for applied baked data. |
| M05-02/03/05 | Provide actual headless slot ownership and exact linear HDR FrameAccumulator; establish final color/AA only after averaging. Declare effect history/FSR use and example-target import policy. |
| M06-03 and M07-03/04 | Explicitly own/connect/draw/reopen the settings UI; rebind borrowed targets on scene changes, preserve safe partial edits and map poses/transforms to real document IDs with unsaved-state hooks. Separate scene/job/renderer generations. |
| M08-01/03/04 | Reuse explicit surfaces/snapshots with required-capability failure before output; retain sources on cancel, use chart-safe exact bake average and frozen settings. Do not instantiate RendererDeferred just to obtain tracing inputs. |
| M09-01/05 | Inventory real consumers before deletion; keep iryku_smaa bytes/licenses and paired draw shader deployment/manual fixtures. Remove production dependencies only after replacement products exist. |
| M10-02/05 | Audit cumulative pages against final signatures and evidence; do not turn continuation acceptance into invented device coverage. |

Each affected milestone summary links this review. The corresponding step files
contain actionable prerequisites/exit additions, not just this impact table.
No new user architecture decision, public namespace migration or milestone order
change is introduced by this review.

## Transition

Deliver M03-10 for lead acceptance, then implement M03-11 cumulative maintained
documentation. M04 starts only after that documentation closure is accepted.
Historical handoffs are preserved; this review supersedes statements that still
call the explicit surface profiles, effect metadata or visible controls pending.

## M03-11 cumulative documentation handoff — 2026-10-17

M03-10 accepted by project lead continuation instruction; backend/scenario details
not specified. [Documentation closure](documentation.md) maps five added and
twenty-four updated maintained pages to actual source, with retained/retired
coverage and future ownership explicit. Canonical references now cover snapshots,
public surfaces, camera motion, effect metadata/settings/schema, histories/denoise,
final SMAA and real example UI/light controls/defaults. Tutorials retain ordered
construction with focused current wiring, not whole-file listings. Production
render/bake guidance remains supported and separately labeled.

Agent checked 344 links/anchors, 73 source/header references, balanced fences,
source-backed constructors/defaults/call sites and diff whitespace. No compile,
shader compiler or runtime/packaging execution. M03-11 remains awaiting lead
acceptance; it is the last M03 step and M04-01 is gated by that acceptance.
