# M01-09 draw extraction and shader bindings

## Queue snapshot and visitors

`draw::RenderQueueVisitor::enqueue(root, engine, queue)` extracts a complete queue or leaves the previous queue intact on failure. It resets traversal state after success/failure, rejects reentrant extraction and includes ancestor transforms when extracting a subtree. Disabled ancestors/branches are omitted by default. World matrices are `ancestor/node world * Drawable::transform() * localSubmeshTransform`. Loaded draw PNUUT geometry is required; production Drawable modes, unresolved source references, unsupported material UV/channel settings and singular/nonfinite/non-affine object transforms fail explicitly.

Every visible submesh/material produces an Item retaining its realized drawMesh, drawMaterial, six Texture owners, optional BLAS and packed MaterialData, plus worldMatrix, nodeId, weak node lookup and submeshIndex. Empty placeholder DrawableComponents without resources are skipped. CPU mesh edits awaiting reload do not replace uploaded geometry. A complete queue has dense, one-based objectId values in visible traversal order, portable to the 24-bit generic query instance ID. Zero is reserved for background. IDs may change next frame when traversal/visibility changes: use stable `(nodeId, submeshIndex)` keys for history/selection, not an object ID alone.

Classification matches existing behavior: transparent+solid goes to SolidTransparent, other transparent goes to Transparent, everything else is Opaque. Hidden submeshes/materials are excluded. `queue(type)` exposes the classified snapshots; `items()` remains canonical object-ID order. `sort(cameraPosition)` stable-sorts both transparent categories back to front by squared distance to the transformed object origin, leaving opaque/canonical order and IDs intact. It does not infer geometric bounds depth or split transparent triangles.

Enabled lights are extracted with the same node transform stack. Local origin becomes position; local +Z becomes normalized world direction. Native base::Light type values/angles, color, intensity, shadow samples, source size and reflection/shadow flags are preserved. More than eight enabled lights fails rather than truncating. The first enabled environment component in traversal is cloned as pure declarative intent. EnvironmentGenerator synchronization still runs outside active acquisition/recording; extraction never uploads HDR or initializes production resources.

`DrawVisitor::draw(frame, queue, category, bindCallback)` binds captured mesh buffers and records indexed submeshes inside the caller's existing rendering scope/pipeline. Callback receives the Item and binds frame/object/pass state. A convenience overload extracts/sorts a root into a caller-retained queue. It does not mutate Drawable's transform, duplicate GPU geometry or use native scene drawing callbacks.

`CollectRayTracingInstancesVisitor` consumes the exact queue, or invokes the same extractor for a root. It retains Items alongside generic RayTracingInstance descriptions, checks BLAS readiness, buffer identity and submesh index ranges, and preserves object IDs/world matrices. On a no-acceleration device it returns an empty collection without affecting raster extraction. Both transparent classes are excluded by default, with independent explicit opt-ins. Opaque alpha-tested materials remain included with their full texture/alpha metadata; M01-11 implements the actual alpha-aware hit table/shader and a supported non-opaque traversal path; current native BLAS geometry still marks triangles opaque. Capacity overflow and missing enabled BLAS throw; enabling tracing after an upload that omitted BLAS requires Drawable reload. `populate(scene)` replaces only that same-device generic scene's CPU instance list; caller records TLAS work and retains this collection through GPU completion. Do not create/update unsupported or empty TLAS resources unconditionally.

## Mutable data ownership and acquired slots

`FrameDataBinding`, `ObjectDataBinding`, `LightDataBinding` and `EnvironmentDataBinding` are public draw classes. `initialize(engine, sharedPipelineLayout, frameSlots)` runs at a stopped-producer boundary. Zero frameSlots derives the current window's in-flight count; explicit positive count supports a caller-owned slot schedule. Use drawBindingLayout() or an exactly compatible layout. Engine/device outlive every queue, binding, external snapshot and submitted consumer.

After acquire/fence waiting and before recording consumers, update the frame/light/environment bindings and call ObjectDataBinding::prepare(frame, queue) once. `set(frame)` returns the current slot's ready GPU ResourceSet; object `set(frame, objectId)` indexes the canonical one-based ID. All use FrameContext::frameSlot and presentation frameNumber. A second preparation for the same presentation number throws, preventing accidental UBO overwrites while recording multiple passes. This guard does not replace fence acquisition: never fabricate another frame number to rewrite an in-flight slot. It is valid to reuse each prepared set across passes/pipelines with compatible layouts.

Frame/object/light/environment UBOs and mutable sets are duplicated per acquired slot. Object slots own per-item records and retain queue resource snapshots; environment slots retain their published environment snapshot. Geometry, images, samplers and BLAS are shared resources, not duplicated frame rings. Object count changes allocate/retire only the current completed slot's data. Preparation performs no waitIdle or resource upload/submission. Cleanup waits only at the explicit stopped boundary. A failed preparation must not be recorded; environment sets are not exposed in partially rebound state.

Resource replacements happen only at a stopped boundary. Re-extract/rebind afterward: direct Texture reload/updateSampler changes its realization and invalidates queues that captured the previous image/sampler pointers. Replacing a Drawable's realized mesh/material texture objects preserves independently retained snapshots. `queue.validate(engine)` rejects stale geometry/texture bindings and wrong engines before drawing/query extraction/preparation. Explicitly calling native cleanup on externally retained resources is outside this contract.

## Versioned shader ABI

`drawBindingLayout()` identifies `draw.bindings.v1`. Public ShaderData structs use only 16-byte matrices/vectors, explicit uint flags and static size/offset assertions. Shared native headers are `draw/lib/bindings.glsl` and `.metal`. GLSL defines DRAW_BINDINGS_VERTEX or DRAW_BINDINGS_FRAGMENT before including to obtain the corresponding stage declarations. Metal exposes DRAW_VERTEX_DATA_ARGUMENTS, DRAW_FRAGMENT_DATA_ARGUMENTS, DRAW_MATERIAL_TEXTURE_ARGUMENTS and DRAW_ENVIRONMENT_TEXTURE_ARGUMENTS for entry signatures. Stage/resource mappings are explicit because the GPU API currently declares one stage per resource; stage aliases share the same actual UBO without requiring backend changes.

| Data | Vulkan set/binding | Metal resource index | Stage |
|---|---|---|---|
| Frame UBO | 0/0; 0/1 | buffer 2; buffer 6 | Vertex; Fragment |
| Object+material UBO | 1/0; 1/1 | buffer 3; buffer 7 | Vertex; Fragment |
| Six material images | 1/2..7 | texture 0..5 | Fragment |
| Six material samplers | 1/8..13 | sampler 0..5 | Fragment |
| Lights UBO | 2/0 | buffer 4 | Fragment |
| Sky/irradiance/prefilter/BRDF images | 3/0..3 | texture 6..9 | Fragment |
| Environment samplers | 3/4..7 | sampler 6..9 | Fragment |
| Environment UBO | 3/8 | buffer 5 | Fragment |

Metal buffer 0 remains geometry/fragment push namespace, vertex push uses buffer 1; textures and samplers have separate index spaces. No stage alias reuses the same Metal index within a set, which the current ResourceSet matcher requires. Four sets, ten sampled images and ten samplers fit the portable raster ABI. Later fullscreen lighting can reuse these structs with appropriate stage/layout declarations and its own G-buffer bindings; do not blindly add a fifth set or collide with existing native indices.

- FrameData: 240 bytes; view, projection, inverse view-projection (three mat4), camera world position, extent/presentation delta/scene delta, presentation lo/hi and scene lo/hi uint counters. packFrame validates finite invertible camera matrices and valid extent/timing. No temporal history or previous matrices are invented in M01.
- MaterialData: 192 bytes; albedo/fresnel tint/sheen color, factors (metalness, roughness, sheen intensity, refraction), emission/alpha cutoff, three vec4 UV scale pairs (albedo/normal, metalness/roughness, AO/emission), two uvec4 UV/flag groups, scalar channels and inversion metadata. uvSets[0] = albedo/metalness/roughness/normal UV selectors; uvSets[1] = AO/emission UV selectors, flags, manual sRGB decode mask. invert = metalness/roughness/emission inversions, requested sRGB metadata mask. Channels = metalness/roughness/AO/emission. Standard layout supports UV 0/1 and channels 0..3. Pending texture realization flags must be resolved before extraction; scalar edits need no texture rebuild.
- Flags: Unlit=1, AlphaTest=2 (cutoff >0), Transparent=4, Solid=8, HasAOTexture=16. AO presence is not evidence of a baked lightmap; M04 adds real baked-map semantics separately. Colors/scalars are copied rather than silently coerced. Six slots are exactly Albedo, Metalness, Roughness, Normal, AO, Emission.
- Texture metadata controls decoding. UNORM textures marked sRGB set the manual RGB decode bit; native sRGB images already decode in hardware and do not set that bit. Requested colorType remains independently recorded. Native sRGB images marked linear are rejected because no compatible reinterpretation view is supplied. Alpha remains linear. Inputs must accurately declare their color interpretation, including data/HDR textures. Shader helpers provide independent UV/scale selection, RGB decode, scalar channel/inversion and implicit or explicit-LOD material sampling; M01-10 supplies actual PBR shading/alpha discard.
- ObjectData: 336 bytes, model+inverse-transpose normal matrices, MaterialData and uint4 identity (objectId, submesh, reserved, reserved). Lights: 16-byte count plus eight 80-byte LightData records = 656 bytes. EnvironmentData: 16 bytes (max prefilter LOD, sky blur, sky visibility, reserved); retains M01-08's linear radiance, PI-inclusive irradiance and BRDF A/B conventions.

## Shared scene and production boundary

Legacy draw/queue/query visitors and scene::vk bindings remain intact for production. New draw APIs have no direct native headers/types or dependencies on render. Update/Input/Transform visitors stay in scene. ResizeViewportVisitor no longer imports render::Engine or names VkExtent2D; its constrained width/height extent template forwards to the existing pure math::Viewport overload, preserving source calls for both native extents and gpu::Size2D. Existing binaries using the removed native overload need the normal rebuild; binary ABI compatibility is not claimed.

## Verification and downstream work

Static checks cover declarations/callers, include/dependency boundaries, vector/matrix ABI packing, native set/stage/index agreement, ownership and slot guards, extraction identity/visibility/sort, ray/raster submesh equivalence, shader pairs/entry names, links and git diff --check. No configuration, compilation, shader compilation or runtime execution by the agent; no CMake changes.

[Manual probes](../../../../tests/draw/bindings/README.md) provide nested extraction/query checks and a paired GPU diagnostic exercising all four sets/six material slots/environment samplers. Lead validation covers actual Vulkan/Metal, acquired slot isolation, scalar/UV/channel/color metadata readback, alpha fields, invalid modes/contexts, replacement, sorting, empty/no-RT scenes and existing production/resize regressions. Backend/runtime coverage remains unspecified. M01-10 implements deferred layers against these real structs/bindings; M01-11 consumes retained instance metadata for alpha-aware queries; M02 picking uses per-submission ID mappings and stable node/submesh identity; M03 extends motion/history/RT tables explicitly.

## M01-10 fullscreen environment indexing

EnvironmentDataBinding::initialize now optionally accepts metalTextureBase (default6 retains this step's ABI). The deferred fullscreen layout uses base8, avoiding its eight G-buffer/background images at native indices0..7. Frame/object/light packing is unchanged. See [raster-layers.md](raster-layers.md).

## M01-11 opacity and dense lookup

RayTracingScene::addInstance now accepts a defaulted generic opacity override and the collector preserves it when populating. SoftShadowPass independently forces non-opaque cutouts while retaining ordinary opaque hits. Its minimal UV/alpha table uses actual dense TLAS order and retains canonical objectId, never filtered objectId-1 addressing. See [query-shadows.md](query-shadows.md); this does not add advanced ray material addressing to the M01-09 raster ABI.
