# TracingSceneResources and TracingSnapshot

Header: `<bg2e/draw/pipeline/TracingSceneResources.hpp>`.
Namespace: `bg2e::draw::pipeline`. These public draw resources consume a RenderQueue;
they do not depend on production render or private RendererDeferred targets.

Construct TracingSceneResources(Device&, capacity) with a capacity in [1,16384]
supported by the device. Choose from Device::resourceArrayCapabilities().profileCapacity
after reserving the effect's direct bindings: AO uses (4,2,13), GI (5,3,15),
reflections (6,3,16). Native execution and output formats are separate prerequisites.
A snapshot shared between effects must match their Device and fit each selected
capacity. Independent effects can select different capacities. A consumer with a
larger layout passes its capacity to `materialResourceSet(layout, bindingCapacity)`;
the binding arrays are padded without rebuilding the snapshot's material tables.

Selection is the exact minimum of 16384, available image entries, available sampler
entries and half the remaining aggregate resource budget after reservations. It
is not rounded to a compiled profile. A 1024-sampler budget gives GI/reflections
1021 entries. The two tables still use one shared capacity; material ownership
remains per object, independent of these execution-time bindings.

prepare(queue) returns a retained TracingSnapshot. Call only at a retired acquired
slot or stopped job boundary. Its immutable packed geometry cache deduplicates
actual DrawMesh identities, not names. Reload creates a new mesh identity. The
snapshot owns source realizations, material/instance buffers, images, samplers and
TLAS, while Device is borrowed and must outlive every exported owner.

`RendererDeferred` owns a common `TracingSceneResources` and prepares one snapshot
per acquired frame whenever an RT effect needs it. Shadows, AO/GI and reflections
consume that same snapshot and TLAS, including both deferred composition layers.
The common capacity is the minimum profile of the renderer's supported consumers,
including currently disabled effects. Consumer-specific descriptor sets and effect
uniforms remain separate; scene buffers and the TLAS are shared.

`prepare(queue, shadowCapacity)` also generates the lightweight shadow alpha table
and its instance buffer from the same collected instance order. There is no
shadow triangle-UV buffer: both native shadow shaders fetch indices and UV0/UV1
from the immutable shared geometry. `ShadowInstanceData.ranges.x` is the shared
first index and `options.z` is the vertex base. `SoftShadowPass`
accepts this snapshot as the final optional argument to `prepare`; omitting it
uses an internally retained `TracingSceneResources` for standalone preparation,
sharing geometry across its completed-slot snapshots. The renderer builds the TLAS before recording
the layers; subsequent consumer `build` calls are no-ops. Snapshots are retained
per frame slot through GPU completion. Immutable material and alpha-metadata generations
survive unchanged camera/transform frames. Immutable completed acceleration generations
also share TLAS, CPU instance vectors and GPU instance buffers across matching
snapshots. Camera state does not enter the acceleration key. Changed transforms
allocate new instance storage and use the complete existing TLAS build path.

Material generation keys retain source identity and captured effective revisions,
including classification and native texture realization. Entries remain in first
encounter order. Value edits allocate a new immutable material buffer; no submitted
buffer is overwritten. Image/sampler tuples are shared independently when ordered
native owners, image generations, admission capacity and fallback owners match.
Thus a scalar edit does not re-encode unchanged arrays. Shadow metadata keys include
geometry identity, dense order, canonical IDs, rebased ranges, UV/scalar alpha data,
classification and alpha-resource identity. Transforms are excluded. Membership or
order changes rebuild the corresponding mappings. Independent editable materials
remain independent; derived table addresses never enter object serialization.

The shadow kernel/layout is `draw.query.softShadows.v5`. Rebuild library and paired
native shaders together before execution. Vulkan set0 contains invocation uniforms0,
normal/depth images4/5, nearest sampler6 and outputs7/8. Set1 contains metadata1,
vertices2, TLAS3 and indices9. Persistent set2 contains alpha arrays16/24. Metal
indices remain uniforms buffer1; metadata/vertices/TLAS buffers2/3/4; alpha records
buffers5/6; indices buffer7; nearest sampler8; normal/depth textures0/1 and outputs2/3.
Capacity reserves remain (2,1,10).

After `build()`, `shadowResourceSet(sharedLayout, capacity, 1)` provides per-snapshot
scene bindings. `shadowAlphaResourceSet(sharedLayout, capacity)` provides persistent
set2, including fully initialized fallback arrays for all-opaque scenes. Each
SoftShadowPass invocation retains its own set0/targets plus both scene and alpha sets.
`SoftShadowPass::setBindingReuseEnabled(false)` creates fresh scene and alpha sets
per invocation for diagnostic comparisons.

| Record | Layout |
|---|---|
| TracingVertex | position/normal/UV0+UV1/tangent, 64 bytes |
| TracingInstanceData | model, inverse-transpose normal, ranges, identity, 160 bytes |
| TracingMaterialData | MaterialData (192), six independent image/sampler indices with padding, 256 bytes |

Packed original indices and submesh firstIndex form indexBase; vertexBase is added
once by hit lookup. Dense filtered TLAS order addresses instance records, including
Vulkan custom index. Canonical frame objectId/submesh is correspondence only.
Normal transforms account for nonuniform/mirrored geometry. Builder checks ranges,
finite/nonsingular transforms and buffer/count/device limits before publication.

snapshot->build(command) transitions the current wrapper's textures even when its
TLAS is reused. New acceleration is recorded once; a matching completed generation
skips the build. Discard the snapshot if recording aborts. resourceSet(layout,set=1)
returns bindings compatible with tracingBindingLayout(capacity,stage,additionalStages).
Scene bindings are created once per layout identity, set index and effective binding
capacity within that snapshot. Bind `materialResourceSet(layout, capacity)` as set2
in addition to scene set1. Logical set1 contains vertex/index/instance/material
buffers0..3 and TLAS4; set2 contains images5/samplers6. Metal buffers2..6 carry direct
data; argument-record buffers8/9 hold images/samplers with record id0 and native
encoder stride. All unused table entries receive valid fallback resources.

Persistent array sets are cached by retained layout identity, route and capacity,
with at most eight entries per native tuple. Service ownership is bounded to the
latest geometry/material/shadow generation plus up to eight acceleration candidates.
Old generations survive eviction through retained snapshots or array handles. Handles retain their native tuple
and layout even after cache eviction or `clear()`, without ownership cycles. Borrowed
layouts are not cached across calls and must outlive their handles. Shared-owned
layouts are retained. Resource validity and captured image generations are checked
before array cache hits; discard snapshots after in-place native recreation.

Returned sets are immutable to consumers. Scene binding/build calls are serialized
on the recording thread; persistent array cache access is synchronized. Retain the
snapshot and all sets through GPU completion. Empty preparation and `clear()` drop
service cache ownership without cleaning up retained resources. Discard snapshots
after aborted recording. `setPersistenceEnabled(false)` on TracingSceneResources
builds fresh immutable material/shadow generations for diagnostic comparison,
without disabling the existing geometry cache.

Acceleration identity includes packed geometry identity, ordered node/object/submesh
correspondence, mesh/BLAS identity and build generation, world matrix, packed geometry
ranges, material address, mask, tracing inclusion and RT opacity. Effective scalar
material edits and texture replacement preserve acceleration when instance addresses
and RT opacity are unchanged. Visibility and transparency exclusions affect collected
membership; changed membership/order invalidates the complete instance-state key.

Readiness is tied to `CommandBuffer::recordingCompletion()`, independently of the
snapshot's recording-local `_built`. Backend `begin()` creates a new ticket; only a
successful queue submission associates its completion record. Nonblocking polling
must confirm normal GPU completion before prepare may reuse a candidate. Recorded,
unsubmitted, reset, failed and abandoned recordings cannot certify completed reuse.
An early identical frame may allocate its own generation while another is pending;
after completion, matching frames reuse a completed candidate. No polling wait or
device drain is added to prepare/build. Errors from submitted work propagate.

The cache holds at most eight candidates, including pending ones. Eviction and
clear only drop service ownership. Callers retain every snapshot and its descriptors
through all GPU readers, as before. Generations retain packed geometry/source buffer
owners and BLAS owners independently of the original source scene. Do not mutate
or rebuild a retained scene/instance buffer. Prepare/build use the recording thread
and the normal graphics-queue path; native queue-family transfers are not introduced.
`setAccelerationReuseEnabled(false)` restores fresh immutable instance buffers and
full TLAS builds per snapshot while leaving phase 05 material persistence enabled.
Refit is not exposed by the current backends and is not needed for unchanged reuse.
The shader ABI stays at tracing v3/shadows v5.

Vulkan divergent indices require enabled nonuniform features; Metal uses Tier-2
compute argument arrays with complete residency declarations on every bind. This
is bounded material addressing, not unrestricted bindless or Metal graphics arrays.

Vulkan uses UInt32 specialization constant ID0 to match each shader array extent
to the layout capacity. AO/GI/reflection use the deployed `_256` filenames as
templates, not a size restriction. Their native shader interfaces are v3; custom
replacements must use specialization ID0 and Vulkan set2 arrays and the unchanged Metal record bindings.

All six material slots retain image/view/sampler and UV/color/channel conventions.
Cutout acceptance is albedo alpha times material alpha >= cutoff at explicit LOD0;
ForceNonOpaque exposes candidates and rejected hits continue behind holes. Both
blended classes are excluded as occluders/secondary geometry by default. Huge scenes
still fail for invalid geometry/buffer ranges. Material image/sampler table overflow
is recoverable: an excess pair gets `UINT32_MAX` in both index lanes, and
`fallbackTextureSlots` counts degraded slots. Shipped Vulkan/Metal `traceSample`
returns white (flat `(0.5,0.5,1,1)` for the normal slot) before any descriptor access
or color decoding. Scalar/tint/alpha/channel parameters remain unchanged; raster
materials and all tracing geometry remain present. Overflowed cutout textures lose
their holes, subject to the retained scalar alpha/cutoff. Already admitted pairs
remain usable even when a table is full; rejected pairs consume neither table.

One warning is emitted per TracingSceneResources overflow episode, not each frame;
successful nonoverflow preparation or clear() resets it. The practical native table
ceiling is 16384 subject to device budgets. Custom compatible tracing shaders must implement the
no-texture sentinel too. Unsupported devices and malformed resources still fail
explicitly; this policy handles only sampled-material table exhaustion.

See [surface profiles](SurfaceInputs.md), [effect capabilities](../../architecture/draw_advanced_lighting.md),
[GPU arrays](../gpu/ResourceSet.md) and [native tracing](../gpu/RayTracingPipeline.md).
