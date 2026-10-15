# M01-11 actual alpha-aware query shadows

## Reviewed lead corrections and acceptance

The lead fixed M01-10's Metal name collision by renaming the texture argument to `irradianceImage`, keeping texture(9), sampler(9), its sample expression, and the sampled float3 irradiance calculation unchanged. The rename preserves shader behavior and is retained in both the original and new query-lighting variant. The explicit CommandBuffer.hpp include in RenderTarget.hpp supplies its public parameter type. The lead reports that these corrections compile; the agent reviewed source behavior and did not compile. M01-10 is accepted under that instruction; runtime/backend coverage is unspecified.

## Public integration and fallback

`pipeline/SoftShadowPass.hpp` defines a real public RenderLayer with settings, available(), status(), prepare(), and the ordinary initialize/attachScene/resize/update/record/cleanup lifecycle. RendererConfiguration.softShadows supplies a retained shared pass or null for the default. RendererDeferred::softShadows() exposes the selected instance and usesQueryShadows() reports the actual selected graph variant. SoftShadowSettings owns enabled, positive bias, finite directionalDistance, and maxSamples (1..64, default16). The pass derives acquired-slot count from its constructor or window surface. Caller-injected slot counts must match the renderer's actual acquired schedule.

Initialization combines logical Device::rayTracingCapabilities().querySupported with sampled/storage/color RGBA16-float usage and successful native compute shader/pipeline/sampler creation. No RT pipeline/SBT or render-stage query capability is required; Metal compute-query hardware can work without rayQueryInRenderStages. Unsupported capabilities/format or query-pipeline creation failures report unavailable status and select the original unshadowed PBR graph. They allocate no query masks or TLAS. This is independent of fullSupported()/rayTracingPipeline. Available means the query pipeline is ready, not that a particular scene has shadow-casting lights/occluders.

With a supported default, lighting selects `draw.raster.deferred.shadows.v1` and two extra RGBA16-float internal visibility targets. With enabled=false, no casting lights, or an empty tracing collection, the targets clear to one and no TLAS build/query runs. Disabled settings do not require Drawable BLAS: a scene can explicitly disable Drawable tracing and still render. Enabling queries requires supported, loaded, built tracing geometry for enabled occluders; the collector rejects stale/unbuilt geometry rather than hiding it. Existing Drawables may have already allocated BLAS during their own normal load; changing query settings does not destroy their owned resources.

An explicitly injected `draw.raster.deferred.v1` lighting replacement opts that renderer graph out of queries even on supported devices and retains M01-10's compatibility contract. Its query pass may be available but usesQueryShadows() is false and no query targets/TLAS are used. A query-aware replacement declares the new exact contract and native input map. Unknown/incompatible query-specific replacements still fail clearly. Runtime enabled toggling retains the selected compatible graph, with white visibility when disabled; changing graph/component choices remains a stopped lifecycle operation. No other pass injection behavior is closed.

The default renderer prepares query tables once after extracting its acquired-slot queue. During recording each opaque/transparent G-buffer is followed by the public compute visibility pass, then the compatible deferred fullscreen composite. Thus transparent receivers use their own current depth/normal data, while default occluders remain opaque/cutout only. Only the corresponding direct BRDF/sheen contribution is multiplied by the matching light's visibility. Ambient IBL, emission, unlit color and final processing retain their existing paths. No AO/GI/reflection/history/denoise/noise-texture object or tracing pipeline is introduced.

## Generic GPU opacity and synchronization

RayTracingMeshDescription appends opaque=true; Vulkan triangle geometry flags and Metal setOpaque honor it. RayTracingInstance appends RayTracingOpacity::Geometry/ForceOpaque/ForceNonOpaque, and RayTracingScene::addInstance accepts it as a final defaulted argument. Vulkan maps overrides to FORCE_OPAQUE/FORCE_NO_OPAQUE, Metal to InstanceOptionOpaque/NonOpaque. Geometry default inherits BLAS opacity; both keep triangle culling disabled as before. Vulkan top-level instance-geometry flags are zero, leaving opacity to instances and BLAS. Existing callers/aggregate prefixes retain defaults and no production render code is migrated.

SoftShadowPass sets per-instance ForceNonOpaque only for cutouts, ForceOpaque for ordinary accepted opaque materials. Existing opaque BLAS therefore need not be rebuilt when a material changes alpha cutoff. GLSL ray queries use TerminateOnFirstHit without the opaque ray flag and confirm only accepted triangle candidates. Metal uses intersection_query<instancing,triangle_data>, per-instance opacity and commit_triangle_intersection with accept_any_intersection. Rejected holes continue traversal; ordinary opaque hits retain the fast automatic committed path. No intersection function table/any-hit pipeline is created.

Each acquired slot owns its collection, CPU query table, TLAS, mapped uniform/storage buffers and all invocation descriptor sets. Geometry/texture/sampler owners are retained by the collection. CPU table upload and TLAS instance edits occur only after actually acquiring/waiting the completed slot. The first visibility invocation records TLAS build through gpu::CommandBuffer; later transparent invocations reuse it. The existing Vulkan AS barrier covers compute reads; Metal encoder transitions/resource declarations provide the native build/read path. Descriptor sets are distinct per invocation, never overwritten while previous consumers are in flight. Visibility images are shared within that slot's ordered graph, with ShaderReadOnly -> General -> ShaderReadOnly barriers between lighting reads and later visibility writes.

No normal prepare/record path waits globally or immediate-submits. Buffer capacity grows only within the completed slot; geometry/textures do not become raster buffer rings. Synchronous lifecycle operations wait and require stopped background producers. As required for mapped query tables, Vulkan createMappedBuffer/updateMappedBuffer now flush VMA allocations after CPU writes, including non-coherent memory. This fixes the generic mapped write path for existing uniforms too, without new staging transfers or waits. Direct callers retain input/output images and all owners until submission completion and use Renderer::recordLayer validation around the public pass.

## Deliberately small alpha lookup table

`buildShadowQueryTable(collection)` exposes the actual CPU representation for inspection/reuse. It does not load resources, build AS or choose an advanced RT architecture. It uses the exact retained DrawMesh CPU snapshot and submesh firstIndex/indexCount; UV triples follow BLAS-local primitive order. No raster vertex/index GPU buffer is copied or reuploaded. Query-only UV metadata is packed into the slot's storage table alongside mutable instance/material lookup data.

| Structure | Size | Fields |
|---|---|---|
| ShadowInstanceData | 48 bytes | ranges uint4 (first UV triangle, triangle count, alpha binding or UINT_MAX, canonical objectId); alpha float4 (scale.xy, cutoff, albedo alpha); options uint4 (UV set, flags, reserved) |
| ShadowTriangleUV | 48 bytes | three float4 values, each UV0.xy/UV1.zw |
| ShadowUniforms | 752 bytes | inverse view-projection mat4 at0; bias/directional-distance/reserved float4 at64; counts uint4 at80; unchanged LightUniforms at96 |

The table is dense TLAS instance order, addressed by GLSL rayQueryGetIntersectionInstanceIdEXT/Metal get_candidate_instance_id. It never indexes by canonical objectId-1 after filtered instances are removed. Canonical objectId remains in ranges.w for correspondence with retained nodeId/submesh identity. Hardware primitive IDs address the submesh-local UV triangle range. Barycentric weights are (1-b.x-b.y,b.x,b.y); interpolate the chosen UV0/UV1, then apply albedo scale. Accepted alpha is explicit-LOD0 texture alpha * material albedo alpha >= cutoff, with the original sampler. Alpha remains linear for UNORM and native sRGB textures; no RGB decoding or scalar-channel inversion applies to it.

Only cutouts need UV triples/alpha bindings. Eight distinct albedo image/sampler pairs can be used across arbitrarily many retained instances within the collector's 24-bit instance capacity. Pairs sharing actual image and sampler deduplicate. A ninth pair throws an explicit capacity error; no occluder is silently truncated or made opaque. Eight statically indexed native bindings avoid dynamic/nonuniform descriptor indexing, resource arrays, bindless address frameworks or a speculative M03 material architecture. Unused bindings receive a retained valid fallback albedo pair. Future larger/advanced tables require the M03 design checkpoint. Both transparent classes remain excluded by default, exactly as M01-09 specified.

## Native resources

The query kernel lives at `draw/query/soft_shadows.comp.{glsl,metal}` and uses the public softShadowBindingLayout, one compute set:

| Resource | Vulkan binding (set0) | Metal index |
|---|---|---|
| ShadowUniforms UBO | 0 | buffer1 |
| instance table SSBO | 1 | buffer2 |
| UV triangle table SSBO | 2 | buffer3 |
| TLAS | 3 | buffer4 |
| normal/emission, depth | 4,5 | textures0,1 (depth2d<float> for Metal depth) |
| nearest fetch sampler | 6 | sampler8 (GLSL texelFetch; unused by native Metal reads) |
| visibility outputs0,1 | 7,8 | storage textures2,3 |
| eight alpha images | 16..23 | textures4..11 |
| eight alpha samplers | 24..31 | samplers0..7 |

Both use 16x16 workgroups with explicit bounds checks. Normal/depth reconstruct the current receiver; cleared normals produce white visibility. Shadow0 RGBA carries light0..3 and Shadow1 carries4..7. Noncasting/unused lanes are one. The kernel offsets origin/min distance using bias, uses a finite directional maximum distance, and accounts for a local light's distance. Directional sourceSize is degrees; point/spot size is approximated by atan(world radius/distance). The sampled cone is bounded to1.4 radians to avoid a tangent singularity. One sample is a hard center ray; larger counts use a deterministic pixel/light rotated disk sequence, capped by maxSamples. Light shadowSamples=0 keeps the prior single-hard-ray behavior. No frame history or denoising is implied.

The query-aware deferred variants are `draw/raster/deferred_shadow.frag.{glsl,metal}`. G-buffer/background remain Vulkan set1 images0..7. Two masks append images8,9; all ten samplers use bindings10..19. Metal G-buffer/background remain textures/samplers0..7, environment remains8..11, and masks use12,13. Frame/light/environment UBO ABIs are unchanged; available variants use deferredBindingLayout(true). FullscreenPass adds an optional explicit Metal input-index map, so inputs8/9 do not collide with environment resources. Existing deferredBindingLayout(false), ordinary final input indices and the lead's corrected unshadowed shader remain unchanged.

Existing paired native source discovery/deployment handles the new files. No CMake edits/configuration/compilation/shader execution were performed. SDK Metal headers were inspected for intersection_query, depth reads and native opacity option spellings; Vulkan query correspondence follows the [GLSL extension](https://github.khronos.org/Vulkan-Site/glslext/latest/glslext/ext/GLSL_EXT_ray_query.html). Apple's [intersection-query documentation](https://developer.apple.com/documentation/metal/control-the-ray-tracing-process-using-intersection-queries) describes candidate enumeration/commit rather than a Metal RT pipeline.

## Validation and pending work

[Manual fixtures/checks](../../../../tests/draw/soft_shadows/README.md) cover table identity/UV ranges, actual cutout transmission and continued traversal, light lanes/sample softness, both transparent classes as receivers, no-RT/forced-disable fallback, explicit capacity failure, acquired-slot lifetime, resize/replacement, native creation/status and shader variants. The previous [deferred pixel probe](../../../../tests/draw/deferred/README.md) remains applicable. The agent performed only static source/include/delimiter/native-binding/table/layout/link/whitespace checks; project lead owns compilation and native runtime checks.

M01-12 reflects live settings and available/status/usesQueryShadows separately; enabled alone does not establish effective support. M01-15 demonstrates these controls and no-RT behavior. M03-01 revisits the eight-pair UV-only mechanism and native opacity before choosing arbitrary material addressing/advanced effects. M03-02 can reuse identity and opacity primitives but must not mistake this minimal alpha table for full ray material/geometry data. M04/M08 still need their actual arbitrary bake shading tables and contexts. Production ray-query examples and render implementations remain untouched references.
