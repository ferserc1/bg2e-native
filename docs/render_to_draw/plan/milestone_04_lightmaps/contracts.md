# M04-01 bake contracts and production mapping

Date: 2026-10-17. Baseline HEAD `95190d6d042bb85d6e9d72153b155192dac64377`.
M03-11 accepted by project lead continuation instruction; backend/scenario coverage
not specified. M03 is closed. This delivery adds CPU settings/pixel validation and
abstract execution contracts, not a working integrated/standalone baker.
Production render/bake implementations remain untouched and supported.

## Delivered public surface

All headers are under lib/include/bg2e/bake; include bake/all.hpp explicitly.
Types.hpp defines LightmapMode, LightmapPixelFormat, BakeState and LightmapSignal.
LightmapSettings owns actual values behind accessors and live metadata type
bg2e.bake.lightmapSettings.v1 with matching settingsInstance(). Metadata binds that
instance; a job freezes a validated copy, not a second editable UI settings model.
Scalar setters enforce finite/range/enum constraints; validate() checks combined
radius/distance > 2*bias and dilation <= resolution. Catalog integrations must
validate merged proposed values before starting/restarting work; no running job
may silently adopt a partial settings edit.

| Setting | Default / allowed range |
|---|---|
| resolution | 512; 1..16384 square target (actual device limits checked by executor) |
| mode | RTGI; RTAO or RTGI |
| accumulationFrames | 16; 1..1048576 exact completed updates, not rolling display history |
| samplesPerPixel | 8; 1..64 rays per update |
| giBounces | 2; 1..8 |
| aoRadius | 0.1 world units; 1e-6..1e9 |
| maxRayDistance | 50 world units; 1e-6..1e9 |
| rayBias | 0.0005 world units; 1e-6..1e9, used for either mode |
| exposureEV | 0; -32..32, GI RGB8 export only |
| cpuFormat | RGB8 or RGB32F |
| dilationPixels | 4; 0..16384, at most resolution |

LightmapPixels is tightly packed row-major RGB. Row zero is top, corresponding to
texture UV v=0; M04-02 native UV projection/readback must preserve that contract.
RGB8 stores linear UNORM bytes; GI byte conversion applies 2^exposureEV then clamps
and rounds; AO byte conversion has no exposure. RGB32F stores finite nonnegative
linear float multipliers above one unchanged. No gamma, receiver albedo, alpha,
premultiplication or display tone mapping. Payload must match exact dimensions and
variant. elementCount/validate reject zero dimensions, overflow, wrong variants/
length, unknown format and negative/nonfinite floats. Conversion/readback are not
implemented by this value type.

BakerContextConfiguration borrows Device and retains scene/environment/pass owners,
explicit backend/frame slots and optional shader root. Structural validate requires
live Device/root/nonzero slots; native execution/profile/format/capacity/environment
validation belongs to concrete preparation before output allocation. Device
outlives context and retained images/snapshots/results. BakePassConfiguration
accepts public RTAmbientOcclusion, RTGlobalIllumination and DenoiseFilter owners,
plus a replaceable UvSurfacePass owner (its full surface interface is M04-02).
The surface primitive must expose compatible WorldPositionCoverage inputs.
Null AO/GI selects executor-owned defaults later; null denoise means no spatial
filter. Injected pass mutable slots belong exclusively to this bake lifecycle,
not a renderer's active effect object. No mandatory RendererDeferred or native
Vulkan descriptor is present.

BakerContext and LightmapBaker are noncopyable abstract contracts. The target
configuration retains node and actual settings. BakeFrame borrows an inactive-scope
CommandBuffer and completed slot/serial/sourceGeneration. No implicit submission.
BakeProgress distinguishes Ready/Running/Complete/Cancelled/Invalidated/Failed,
completed GPU updates, requested updates and source/target generation. A returned
optional result is present only after current completed composition/dilation;
readPixels throws for missing/incomplete/stale results. Reset invalidates the
previous target generation; source edits invalidate context generation. Cancellation
retains submitted owners until completion; cleanup runs stopped with Device alive.
Concrete implementations of these operations arrive in M04-03/M04-04.

LightmapResult carries the retained floating sampled GPU image, extent, mode,
explicit IndirectMultiplierRGB signal, UV set 1 (texCoord1/UV2), source/target
generation. Image precision/capabilities must be checked by the executor and HDR
overflow rejected, not clipped. Presence comes from a current published result and
application metadata, never an AO texture flag or RGB being nonwhite.

## Actual draw bridge and composition policy

Use SurfaceInputs::WorldPositionCoverage: signed RGBA16 normals, RGBA32 world
positions, separate R8 coverage, matching sampled surface extents. AO raw signal is
R16 visibility neutral one. GI raw signal is RGBA16 incoming diffuse radiance/pi
before receiver albedo, alpha validity. Preserve valid black versus invalid zero.
Required active effect/profile/capacity failures must fail the job, never publish
white as a successful unavailable bake. An empty geometry GI snapshot currently
returns invalid: reject environment-only GI jobs until an explicit estimator is
implemented, rather than silently count invalid samples. Empty AO occluders mean
unoccluded visibility one. Covered invalid GI samples cannot advance successful
convergence; M04-04 must specify exact per-pixel valid counts and failure reporting.

Use retained matching-Device/capacity TracingSnapshot from TracingSceneResources;
validate before publication and discard on aborted recording. Sample key advances
for actual bake samples, not UI/presentation frames. Executor sets the public
AO/GI mapped settings from the frozen target settings: enabled, sample count,
AO radius/bias or GI distance/bias/bounces. Bake signals use full target extent
(Ultra for shipped public effects); reject incompatible injected dimensions.
Preserve additional supported injected noise/shadow controls. No display temporal,
FSR or SMAA state enters exact raw UV sums. Shared environments/lights are captured
as immutable scene inputs; required GI environment is validated before recording.

AO composition is vec3(visibility). GI composition is componentwise
(incomingDiffuseRadiance/pi)/(unoccludedEnvironmentIrradiance/pi): zero reference
channels produce zero, not infinity. Receiver albedo/PI are not applied again.
Uncovered/unbaked output is neutral white, separate from valid black. Dilation
extends valid edge multipliers into atlas padding and affects GPU/CPU consistently.
Chart isolation precedes optional filtering/dilation: current DenoiseFilter has no
chart-ID input and cannot be applied across nearby/coincident charts unchanged.
M04-04 must supply an extension or isolated invocations; raw sums remain separate.

## Complete legacy responsibility mapping

| Existing production facility | New bake responsibility / owner |
|---|---|
| render::LightmapSettings fields | bake::LightmapSettings accessors/metadata delivered here; rayBias replaces giRayBias and explicit aoRadius replaces hidden fixed AO radius |
| LightmapPixels in render/LightmapBaker.hpp | Dedicated bake/LightmapPixels.hpp with checked payload/domain/orientation, delivered here; no production alias/removal |
| render::BakerContext | Device/scene/environment ownership and source generation abstract base here; integrated M04-03, standalone M08-01 |
| render::LightmapBaker / IntegratedLightmapBaker / StandaloneLightmapBaker | Abstract bake::LightmapBaker target contract here; recording/progress in M04-03, actual accumulation/readPixels in M04-04; standalone scheduling M08 |
| private render/UvSurfacePass | Public bake UV surface primitive, depthless transforms/coverage/chart identity, M04-02 |
| private render/LightmapBakeExecutor | Bake-owned orchestration over public draw effects/snapshots, M04-03; no copied native tracing implementation |
| private render/UvTemporalAccumulator | Bake exact sample sums/counts, M04-04; not draw::TemporalAccumulator |
| private render/LightmapCompositionPass | Bake normalized RGB composition and consistent dilation/readback, M04-04 |
| render::UvMapPreviewRenderer and ui::UvMapPreview integration | bake preview renderer/widget bridge M04-05, forward name only now; no preview implementation promised |
| render::StandaloneBakerContext | bake::StandaloneBakerContext, forward declaration only; M08-01 |
| render::StandaloneBakeSceneAssembler / StandaloneBakeBatch | bake::SceneAssembler / Batch, forward declarations only; M08-02/03 |
| geo::GenerateUv2AtlasModifier / app safe reload | Stay in geo/app, executed at stopped producer boundary with new draw/tracing mesh identity, M04-02 |
| db::LightmapOutputWriter and generic model/image IO | Remain generic IO; adapt typed bake pixels at M04/M08 output integration without moving scheduling into DB |

These public-name refinements do not migrate production or implement M08 early.
Abstract base/target contracts do not require future standalone classes to run an
integrated bake. Concrete scheduling, UV pass, filters, result binding and pixels
remain assigned to their next steps.

## Verification and handoff

Agent performs source-only checks, not compilation/runtime. Manual CPU fixture
[tests/bake/contracts](../../../../tests/bake/contracts/README.md) checks live metadata,
strict codec round trip, scalar/coupled validation and typed RGB/HDR/overflow errors.
Lead must compile/link new library sources on supported platforms, invoke fixture
from an existing temporary harness and verify required symbol export. No new CMake
or test target is added. No GPU bake, preview or shader output exists in this step.

Next M04-02 implements actual UV surface/coverage/chart primitives using these
formats/generations; M04-03/04 implement the abstract contracts. Native numeric,
completion and application checks stay explicitly pending until those deliveries.
