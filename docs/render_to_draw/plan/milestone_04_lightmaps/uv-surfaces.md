# M04-02 — UV2 replacement and explicit surface implementation

Baseline HEAD: `95190d6d042bb85d6e9d72153b155192dac64377` plus preserved M04-01
uncommitted work. M04-01 accepted by project lead continuation instruction;
backend/scenario coverage not specified. No compiler, shader compiler, renderer,
fixture or packaging execution by the agent. Production render implementation,
geo atlas generator/validator and GPU primitives are reused, not migrated.

## Delivered APIs and lifecycle

- bake/UvSurfaceMesh.hpp: pure CPU buildUvSurfaceMesh validates texCoord1 using
  geo::UvAtlasValidator, creates deindexed native records and conservative charts.
- bake/UvSurfacePass.hpp: Device/backend/slot initialized public pass, virtual
  initialize/prepare/beginFrame/record/cleanup and configurable shader root/prefix/
  interface. Matching ABI is bake.uvSurface.v1. Initialize/prepare/cleanup require
  stopped producers; initialization can be partial, so cleanup on failure too.
- bake/Uv2Reload.hpp: synchronous generateUv2AndReload stages a CPU mesh, generates
  UV2 in geo, validates, deep-clones actual target resources and reloads a fresh
  draw geometry/available BLAS realization. Returns an independent Drawable and
  atlas report. Source remains unchanged; no rollback of a partially mutated
  source is needed. Caller publishes/re-extracts and invalidates generations.
- app::Uv2SafeReload::regenerateDraw weakly queues this operation against the draw
  delegate generation, scene root, target Drawable and uploaded mesh identity.
  SafeUpdateToken cancels pending work; token expiry is checked again before
  publication/report. beforeChange must invalidate any active bake/source state
  before mutation, even if a later operation fails. The callback must be a safe
  invalidation hook, not a substitute scene-adoption operation.

The draw delegate's update boundary drains Device users. Renderer histories are
invalidated and beforeChange runs before generation/publication. Regeneration
only replaces the target node's draw Drawable after complete success; component
adoption preserves the same draw Engine. Completion refetches selection/preview/
settings resources, and the delegate requests scene work. Expired, moved, replaced
or stale targets report failure rather than editing a different object. Callback
errors are not caught/reported twice. Production regenerate() is unchanged.

The independent replacement intentionally includes deep material/texture/GPU owners
through Drawable::clone, preserving GPU-authored data and shared-reference safety.
This is a rare stopped edit, not routine per-frame instancing or an implicit
ResourceManager. Existing node/component identity is not the bake generation;
UV-only edits require explicit source/target invalidation regardless of pick IDs.
Topology/material association and local submesh transforms remain intact. UV2
preprocessing stays geo; raw GPU reload/snapshot/selection consumers must rebind.

## Surface preparation, charts and ownership

prepare(target) validates a loaded same-Device Drawable and its actual uploaded
DrawMesh data, not uncommitted CPU edits. It drains at this explicit preparation
boundary, checks storage-buffer size, validates ranges/UV coverage and packs an
immutable storage buffer. Retain its UvSurfaceGeometry owner. Geometry snapshots
retain Drawable, uploaded DrawMesh and packed vertices. A reload/cleanup of that
Drawable invalidates recording with that snapshot; prepare again.

64-byte vertex records contain four 16-byte lanes: position, normal, UV2 and uint
identity(submesh,chart,reserved,reserved). Triangles are deindexed for constant
per-primitive identities. Tiny/zero mapped area follows validator threshold
1e-12 and covers no texels; mapped positive-area geometry requires finite positions,
nonzero finite normals and nonzero 3D area. Submesh ranges address vertices, not
an index buffer. FirstVertex includes each submesh's offset exactly once.

Charts join shared source vertex-index edges within each submesh; per-vertex UV
continuity is inherent. Duplicated boundary vertices are never welded across a seam. IDs are one-based, stable within
the prepared geometry only; different material/submesh transforms are isolated.
This conservative chart partition can split a generated atlas chart at a submesh
boundary; it is not the generator's chart count or a persistent document ID.
Non-overlap validation rejects coincident positive-area UV triangles. Even touching
islands with duplicate seam vertices stay separate; conservative splitting is safe.
No chart IDs are inferred from material identity alone. M04-04 must explicitly
consume chart IDs for filtering/dilation; current draw DenoiseFilter has no guide.

beginFrame(slot,serial) is called once after that acquired/job slot completes.
record accepts current snapshot, nodeWorld (excluding Drawable/submesh transforms),
extent and source/target generation tags. Caller/executor validates generation
chronology; this primitive carries tags and checks geometry ownership, not scene
root/job validity. Combined model=nodeWorld*Drawable.transform*localSubmeshTransform.
All affine/normal transforms and representable vertex values are checked before
recording. Inverse transpose handles nonuniform/mirrored transforms; no camera or
picking buffer is used. All geometry submeshes are rasterized independently of
viewport visibility; coverage is geometric UV coverage, not receiver alpha cutout.

Fresh output images, immutable per-draw uniforms/descriptors and geometry owners
are retained per invocation and completed slot. Multiple targets can record in the
same slot/serial (maxInvocations default 64). No output aliasing, slot-global mutable
transform buffer, per-record waitIdle or hidden submission. Keep Device alive
through every retained geometry/result owner, even after cleanup. Caller must
retain/submit in order; discard aborted recordings and reset the surrounding job.

## Native profile and orientation

UV v=0 maps to top image row on both backends using clipY=1-2*v, matching Vulkan's
negative-height viewport and Metal's top-left viewport. This is the new bake pixel
contract; do not copy production UV-surface v=1-at-top projection blindly.

| Output | Format | Clear / meaning |
|---|---|---|
| images[0] | RGBA32 float | World XYZ, covered W=1; zero uncovered |
| images[1] | RGBA16 float | Signed unit world normal, W=1; zero uncovered |
| images[2] | R32_UINT | Zero-based submesh/material ordinal, UINT_MAX uncovered |
| images[3] | R32_UINT | One-based chart, zero uncovered |
| images[4] | R8_UNORM | Exact 255/0 geometric coverage |

Four depthless, blend-disabled, no-cull MRTs total 32 attachment bytes/sample.
Coverage is derived from integer chart in a separate bounded 16x16 compute pass,
so five MRTs/33 bytes are not required. Required native formats/usages, four-MRT
budget, image dimensions and storage range are checked. Unsupported required
profiles throw, never pretend an empty/white bake succeeded. No RT is required.

UV graphics set0: UBO0 -> Metal vertex buffer2 (model + inverse-transpose, 128 bytes),
storage vertices1 -> buffer3. GLSL explicit varyings match named Metal varyings;
integer identity is flat. Coverage set0: sampled chart0 -> texture0, nearest sampler1
-> sampler0, storage coverage2 -> texture1. Raster outputs transition to sampled
before coverage; R8 transitions storage then sampled. No native API is exposed.
Shaders live in shaders/src/bake/uv_surface, paired surface vertex/fragment and
coverage compute files. No CMake edits or new target are needed under source globbing.

UvSurfaceResult::surfaceInputs validates and borrows the public
SurfaceProfile::WorldPositionCoverage profile from owned position/normal/coverage.
AO/GI/filter consumers keep the full result alive through completion. Required
raw AO R16 and GI RGBA16 meanings remain unchanged. Submesh/chart remain available
for composition/isolation but are not part of today's draw denoise inputs.

readDiagnostic only reads a completed submitted result: existing color readPixels,
explicit memcpy of packed float32/uint32 and IEEE half conversion for normal data.
No depth readback, implicit format conversion or extra GPU staging API. Result
owners alone do not prove submission/completion; calling before submit is invalid.

## Validation and remaining work

[Manual fixture](../../../../tests/bake/uv_surface/README.md) covers CPU chart
adjacency/material partition and malformed atlases, then numerical 33x17 coverage/
orientation, mirrored/nonuniform world transforms, signed normals/typed integer
clears, two invocation independence, retained output across slot reuse and safe
UV2 replacement/clone/optional BLAS identity. Agent performed static source/ABI/
include/license/lexical, binding/profile/order, callback and local-link checks only.
Lead compiles engine and both native shader sets and invokes fixtures; no passing
runtime outcome or backend parity is inferred here.

Next M04-03 implements concrete bake context/target preparation and generation
validation over these actual surfaces and matching-capacity tracing snapshots.
M04-04 adds exact raw accumulation, chart-safe filtering/composition/dilation and
completed typed output. M04-05 owns preview integration; no GPU lightmap generation
or preview product is claimed by the surface primitive.
