# TracingSceneResources and TracingSnapshot

Header: `<bg2e/draw/pipeline/TracingSceneResources.hpp>`.
Namespace: `bg2e::draw::pipeline`. These public draw resources consume a RenderQueue;
they do not depend on production render or private RendererDeferred targets.

Construct TracingSceneResources(Device&, capacity) with a supported fixed profile
32, 128 or 256. Choose from Device::resourceArrayCapabilities().profileCapacity
after reserving the effect's direct bindings: AO uses (4,2,13), GI (5,3,15),
reflections (6,3,16). Native execution and output formats are separate prerequisites.
A snapshot shared between effects must match their Device and selected capacity;
independent effects can select different capacities.

prepare(queue) returns a retained TracingSnapshot. Call only at a retired acquired
slot or stopped job boundary. Its immutable packed geometry cache deduplicates
actual DrawMesh identities, not names. Reload creates a new mesh identity. The
snapshot owns source realizations, material/instance buffers, images, samplers and
TLAS, while Device is borrowed and must outlive every exported owner.

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
Keep snapshot and returned descriptors alive through GPU completion. Never mutate
an in-flight snapshot or rebuild the same one across independent submissions.
clear() drops cache ownership; externally retained snapshots remain valid.

Logical set1 binds vertex/index/instance/material buffers0..3, TLAS4, images5 and
samplers6. Metal buffers2..6 carry direct data; argument buffer8 has image IDs0 and
sampler IDs starting at capacity. All unused table entries receive valid fallback
resources. Vulkan divergent indices require enabled nonuniform features; Metal
uses Tier-2 compute argument arrays and residency declarations. This is bounded
material addressing, not unrestricted bindless or general Metal graphics arrays.

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
successful nonoverflow preparation or clear() resets it. The fixed native table
profiles are unchanged. Custom compatible tracing shaders must implement the
no-texture sentinel too. Unsupported devices and malformed resources still fail
explicitly; this policy handles only sampled-material table exhaustion.

See [surface profiles](SurfaceInputs.md), [effect capabilities](../../architecture/draw_advanced_lighting.md),
[GPU arrays](../gpu/ResourceSet.md) and [native tracing](../gpu/RayTracingPipeline.md).
