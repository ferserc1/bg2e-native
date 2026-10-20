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
larger layout passes its capacity to `resourceSet(layout, 1, bindingCapacity)`;
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
per frame slot through GPU completion. Material/instance/shadow tables and the TLAS
are deliberately regenerated each frame, not cached across frames or refitted.
The existing immutable packed-geometry cache is unchanged.

The indexed shadow kernel/layout is `draw.query.softShadows.v3`. Rebuild both
native shadow shaders together with the library. Vulkan set0 binding2 / Metal
buffer3 now supplies shared vertices; Vulkan binding9 / Metal buffer7 supplies
shared indices. The independent alpha-texture table keeps its existing ordering
and scalar fallback policy, with capacity reserves (2,1,10) for the extra index
buffer. Alpha image/sampler bindings remain Vulkan16/24 and Metal records5/6.

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

snapshot->build(command) records TLAS construction once for that recording and
submission; discard the snapshot if recording aborts. resourceSet(layout,set=1)
returns bindings compatible with tracingBindingLayout(capacity,stage,additionalStages).
Bindings are created once per layout identity, set index and effective binding
capacity within that snapshot. Repeated effect invocations (including transparent
compositions) receive the same ResourceSet without resolving image/sampler arrays,
allocating argument buffers or encoding native tables again. Shared-owned layouts
are retained to prevent cache-key address reuse; otherwise callers must keep the
layout alive. Returned scene sets are shared and must not be mutated or cleaned up
by consumers. Calls are serialized on the recording thread. Each new frame/job
snapshot starts with an empty cache, so no cross-frame caching is introduced.
Keep snapshot and returned descriptors alive through GPU completion. Never mutate
an in-flight snapshot or rebuild the same one across independent submissions.
clear() drops cache ownership; externally retained snapshots remain valid.

Logical set1 binds vertex/index/instance/material buffers0..3, TLAS4, images5 and
samplers6. Metal buffers2..6 carry direct data; separate argument-record buffers8/9
hold images/samplers with record id0 and native encoder stride. All unused table entries receive valid fallback
resources. Vulkan divergent indices require enabled nonuniform features; Metal
uses Tier-2 compute argument arrays and residency declarations. This is bounded
material addressing, not unrestricted bindless or general Metal graphics arrays.

Vulkan uses UInt32 specialization constant ID0 to match each shader array extent
to the layout capacity. AO/GI/reflection use the deployed `_256` filenames as
templates, not a size restriction. Their native shader interfaces are v2; custom
replacements must use specialization ID0 and the new Metal record bindings.

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
