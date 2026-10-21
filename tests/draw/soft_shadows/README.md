# Manual query-shadow checks

The agent has not compiled or executed these fixtures. No CMake target is added. Compile/run through an existing initialized Engine/DefaultRenderLoopDelegate runner and actual acquired slots. Keep all renderer/scene/images alive through completion; stop producers for initialize/attach/resize/cleanup.

1. Run `runShadowTableProbe(engine)` from probe.hpp outside an active frame. It checks sparse canonical IDs versus dense TLAS order, a cutout submesh with firstIndex=3, two UV sets, scale/cutoff/albedo multiplier, generic opacity and transparent exclusion. No shaders are needed for this fixture; Drawable realizes optional BLAS through its ordinary load path. On no-RT hardware the collection must be empty.
2. Set the RendererDeferred identity camera and attach `shadowProbeScene(engine)`. Inspect `softShadows()->available()` and status(); on a supported logical query device both native compute libraries must load. The first caster texel is a hole and the second is solid. With the point light at (-0.6,0,-0.5), the visible receiver has a shifted shadow near the right side, separate from the caster silhouette. The hole transmits direct light and the solid texel occludes it. Set cutoff=0 at a stopped edit boundary: the caster becomes opaque to queries regardless of texture alpha.
3. Set albedo UV1 different from UV0, use UV1, scale/repeat/mirrored/clamp samplers, change alpha multiplier and cutoff, and verify hole boundaries follow the actual raster material metadata. Alpha is linear for UNORM/native-sRGB images. Queries intentionally sample explicit LOD0 with the material sampler, matching the shared ray-alpha convention rather than screen-space implicit raster mip derivatives.
4. Add a nearer alpha hole and a farther solid caster. Rejected candidates must continue traversal and hit the solid caster. Mix normal opaque BLAS with per-instance ForceNonOpaque cutouts. Compare generic geometry opaque=false and Geometry/ForceOpaque/ForceNonOpaque overrides on both native backends. Keep geometry and instance descriptors alive; no force-opaque ray flag may bypass alpha rejection.
5. Add nested/transformed instances sharing geometry, disabled branches, hidden submeshes and both transparent classes. Only visible default opaque/cutout occluders cast shadows; both transparent classes may receive shadows. Their per-item G-buffers compute fresh visibility before each deferred composite. Reordering nodes, cloning/reloading buffers, or filtering a preceding instance must not remap its UV/material to another canonical ID.
6. Change castShadows, sourceSize and shadowSamples for omni/spot/directional lights. Point/spot sourceSize is a world-radius angular approximation; directional sourceSize is degrees, with the cone limited below 90 degrees. One sample gives a center hard ray. Multiple samples use a deterministic pixel/light disk rotation; no histories/denoise/noise assets exist. Samples clamp to settings.maxSamples (default16, configurable1..64); a light's zero samples retains a single hard ray. Test near-light distance and bias, sourceSize=0, odd internal extents and reduced output.
7. Check light lanes0..7: shadow0 RGBA covers lights0..3 and shadow1 covers4..7, unused/noncasting lanes are one. Only matching direct radiance is attenuated; IBL, emission and unlit remain unchanged. Use a GPU capture or a separately constructed SoftShadowPass with caller-owned sampled/storage/color/TransferSrc visibility images to read the masks after completion. The renderer's internal masks need not be exported for this check.
8. Force settings.enabled=false and render `shadowProbeScene(engine, false)`: no Drawable BLAS is required and no TLAS is created/built. Re-enable on a stopped scene with realized tracing geometry and verify shadows resume. No-RT/query-kernel unavailable/unsupported visibility format uses the original unshadowed PBR pipeline and allocates no query targets/TLAS. Test Metal with compute queries but no render-stage query capability. Missing query library reports unavailable status instead of replacing ordinary rendering.
9. Use more than eight distinct cutout pairs; they must all retain alpha-aware shadows below the selected alphaTextureCapacity(). The device-clamped ceiling is 16384 with reserves (2,1,10), exposed as alphaTextureLimit in graphics diagnostics. Rebuild v3 query kernels: Vulkan specialization ID0, arrays at bindings16/24; Metal argument-record buffers5/6. Shared vertices use Vulkan binding2 / Metal buffer3, shared indices binding9 / buffer7. The table fixture also forces capacity9 and checks bounded indices, per-slot fallback accounting and retention of all instances. Exceed the effective native capacity separately: warn/status without disabling all shadows, scalar alpha/cutoff preserved for omitted samples, and admitted cutout holes still respected. Verify warning rearming after recovery and no per-frame log spam. This changes binding capacity, not material defaults or texture-count policy.
10. Cycle real acquired slots with transforms/material edits, transparent composites, resize/minimize/restore, pause/resume and renderer replacement. Native validation must show no UBO/TLAS/descriptor overwrite while in flight. Normal prepare/record performs no global idle or immediateSubmit. Metadata buffers belong to each acquired-slot snapshot; immutable geometry is shared across snapshots, including independent SoftShadowPass use. Query descriptors retain each invocation through completion. The Vulkan mapped-buffer path flushes noncoherent writes.
11. Inject a compatible query DeferredLayer with `deferredContract(depth,true)` and the actual native map, or inject an explicit unshadowed deferred.v1 replacement to opt that graph out of queries. Other public stages remain replaceable. Unsupported query-specific replacement contracts fail explicitly; default renderer fallback remains functional. Keep all production/native examples operational.

The v3 query pass additionally requires nonuniform sampled arrays/Tier-2 argument
buffers; unavailable query/array/format/shader support must report status and retain
ordinary raster lighting, not masquerade as a passing shadow test. Run Composer's
primitives_scene and Model Edit with the same lighting, more than eight pairs,
no-RT cases, reload/resize and available Metal/Vulkan diagnostics. Source probes
are delivered, not executed. Texture/sampler-count optimizations are explicitly
deferred until the API migration is complete.

The table probe now checks indexed UV0/UV1 fetches and barycentric interpolation
against source triangles for multiple meshes/submeshes, including nonzero packed
vertex/index bases, repeated immutable geometry and empty snapshots. The GI probe
also checks identical shadow/tracing geometry bases. These probes are updated but
have not been compiled or executed. Run both native backends and the public
standalone pass as well as the renderer integration. CPU profiles must no longer
contain shadow triangle packing/upload regions; metadata and binding regions stay.

Phase 04 introduced `draw.query.softShadows.v4`; rebuild the
library and both native query shaders before running these checks. Vulkan stable
bindings1/2/3/9/16/24 move to set1; invocation bindings0/4/5/6/7/8 stay in set0.
Metal native indices are unchanged. The earlier v3 map in checklist item 9 is
historical. Also run the [shadow binding probe](../shadow_bindings/README.md) and
compare multiple transparent compositions with binding reuse enabled/disabled.

Phase 05 advances the active contract to `draw.query.softShadows.v5`, with alpha
arrays in set2. Unchanged shadow metadata buffers now persist across snapshots.
The library and paired native shaders must be rebuilt together before execution.
