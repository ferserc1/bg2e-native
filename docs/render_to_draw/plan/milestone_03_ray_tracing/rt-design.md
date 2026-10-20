# M03-01 — Advanced tracing design checkpoint

Date: 2026-10-16. Source baseline: `15f67dff9efad51c878740bc1ddbce94f880ed46`; working tree clean at entry.

[Step](step_01_rt-design-checkpoint.md) · [Progress](../progress.md) · [Architecture](../architecture-decisions.md)

## Decision status

The project lead **accepted this recommended design by continuation to M03-02**. The findings below retain the M03-01 source baseline; see [M03-02 implementation](rt-gpu-data.md) for the subsequently delivered APIs and remaining work. The continuation selected the recommendation below. Future deviations must update the concrete design and affected pending steps. M02 is accepted: the lead reports that manipulation works on macOS Metal and Linux Vulkan; no device models, logs or individual scenario matrix were supplied.

Recommend native Vulkan raygen/miss/closest-hit plus cutout any-hit, Metal compute/intersection-query traversal, packed indexed geometry buffers, and bounded independently addressed sampled-image/sampler tables. Keep query AO and existing query shadows independent of native Vulkan pipeline support. Preserve native shader authoring and explicit backend bindings. Do not build a general bindless asset manager, emulate Vulkan stages on Metal, or adopt the eight-pair shadow table as the material ABI.

## Source findings, rather than aggregate support

| Inspected implementation | Actual capability / gap |
|---|---|
| `lib/include/bg2e/gpu/RayTracingPipeline.hpp`, `lib/src/bg2e/gpu/vk/RayTracingPipeline.cpp` | One raygen, optional miss and closest-hit, one triangle hit group, internal SBT; no any-hit field, callable stage or multi-group selection. Vulkan currently permits missing stages, despite the API documentation calling miss/closest-hit required. |
| `lib/src/bg2e/gpu/metal/RayTracingPipeline.cpp` | Creates a compute pipeline from the raygen module; ignores miss/closest-hit. No native Metal RT pipeline or hit-function table exists in this wrapper. |
| `lib/include/bg2e/gpu/Common.hpp`, both `PipelineLayout.cpp` implementations | ShaderStage is a single enum, not a visibility bitmask. AnyHit is absent; all switches, push validation, shader loading and merged native descriptor visibility need deliberate extension. |
| `lib/include/bg2e/gpu/ResourceSet.hpp`, both `ResourceSet.cpp` implementations, Metal `CommandBuffer.cpp` | Scalar image/sampler/buffer setters; Vulkan writes descriptorCount=1; Metal directly binds entries to native slots. A layout count field alone does not implement resource arrays or argument buffers. Buffer bindings currently use whole buffers at offset zero. |
| `lib/src/bg2e/gpu/{vk,metal}/PhysicalDevice.cpp`, `gpu/PhysicalDevice.hpp` | Query/pipeline/acceleration/address flags are available, but do not prove material-array support, enabled logical-device features, shader creation or format compatibility. Metal's pipeline boolean describes this wrapper, not Vulkan-style hardware stages. |
| `draw/pipeline/SoftShadowPass.cpp`, paired `draw/query/soft_shadows.comp` shaders | Dense TLAS-order instances, cutout-only CPU UV triples, eight static albedo image/sampler pairs, explicit overflow. Opaque geometry bypasses candidate alpha; cutouts force non-opaque instances. |
| `draw/RenderQueue.hpp`, `draw/ShaderData.hpp`, `draw/Drawable.hpp` | Retained meshes/materials/six texture owners, world transforms, submesh identity, 192-byte MaterialData and CPU MeshPNUUT data exist. Full tracing lookup, previous-object transforms and revisioned tracing snapshots do not. |
| `doc/api/gpu/RayTracingPipeline.md`, `doc/tutorials/gpu/13_ray_tracing_pipeline.md` | Existing wrapper/tutorial only. Cornell Box normals are analytic and its instance attributes are not arbitrary mesh material lookup. Its aggregate support check is not a proposed advanced effect availability test. |
| Production `render/deferred/RTGlobalIllumination.cpp`, `shaders/src/glsl/rt_gi.{rgen,rchit}.glsl` | Behavior reference: iterative bounces, textured albedo, emission, direct lights, cutout visibility and environment; GI hit normals intentionally omit normal maps. Read as reference; no production changes. |

The Vulkan SBT implementation also needs static hardening before reuse: hit offset assumes a miss slot even when no miss group was emitted, host writes have no explicit noncoherent flush, and allocation/base/record alignment must satisfy device properties. M03-02 must validate stage/device/layout compatibility, recursion/dispatch limits and failure cleanup, rather than merely append an any-hit module.

## Backend comparison and execution

| Concern | Vulkan recommendation | Metal recommendation |
|---|---|---|
| AO visibility | Existing compute ray queries, with shared cutout lookup | Compute intersection queries, with the same logical cutout predicate |
| GI / reflections | Native raygen + miss + one triangle group with closest-hit and any-hit | Native compute kernel with nearest committed intersection query and explicit hit/miss shading functions |
| Cutouts | Any-hit calls ignoreIntersectionEXT for rejected triangles; compute queries confirm accepted candidates | Iterate candidates, commit accepted triangles, continue traversal; nearest-hit shading must not use accept-any termination |
| Bounces | Iterative loop in raygen; closest-hit returns hit data/radiance, no recursive transport | Iterative loop in compute; hit data from committed intersection |
| Light visibility | Compute/query or hit-stage ray query with appropriate stage visibility | Nested query function in compute |
| Resource addressing | Fixed-size descriptor arrays with nonuniform sampled-image/sampler indices | Tier-2 argument-buffer texture/sampler arrays with explicit resource residency |
| No support | AO may remain available on query-only hardware; GI/reflections report unavailable | Compute queries and table requirements checked independently; no render-stage-query requirement |

Use one hit group for engine triangles, not one SBT record per material. Dense instance lookup supplies the material; no callable/procedural intersection stages or intersection-function tables are required. Keep the existing GPU example wrapper working. Expose AnyHit and a nullable any-hit description field for Vulkan; Metal's advanced shaders remain independently authored compute kernels with no ignored-stage emulation promise. Use matching paired library paths under `draw/rtao`, `draw/rtgi`, `draw/rt_reflections`, and shared native helpers under `draw/lib`. M03-02 must inspect current shader discovery and document any build integration required; it must not modify CMake without explicit authorization.

## Geometry and identity proposal

A public draw::pipeline tracing snapshot/service (working name `TracingSceneResources`) owns table buffers, sets, TLAS and retained source owners. It accepts collected draw items; GPU receives only primitive resources. It must be reusable without RendererDeferred, UI, a main camera or a swapchain. Proposed names are implementation targets, not existing declarations.

Use four packed read-only storage buffers: vertices, uint32 indices, instances and materials. Pack each unique retained CPU mesh once per snapshot, retaining submesh firstIndex and triangle count; copy original indices with an explicit vertex-base offset. Match BLAS primitive `p` to `indexBase + 3*p`, then add vertexBase exactly once. Deduplicate geometry by actual retained mesh identity, not material name; clone ownership stays unchanged. Validate every range, multiplication, offset, uint32 conversion and device buffer-range limit before publication. No shader raw vertex pointers or per-mesh descriptor arrays are required.

Proposed version-1 records use only 16-byte vectors/matrices (explicit C++/GLSL/Metal layouts, static assertions in M03-02):

| Record | Proposed fields / size |
|---|---|
| Vertex | position float4, normal float4, UV0/UV1 float4, tangent float4: 64 bytes |
| Instance | model mat4, inverse-transpose normal mat4, ranges uint4 (indexBase, vertexBase, triangleCount, materialIndex), identity uint4 (canonical frame objectId, submesh, reserved, reserved): 160 bytes |
| Material | Existing MaterialData (192 bytes), two uint4 image-index vectors and two uint4 sampler-index vectors (six slots plus padding): 256 bytes |

Instance order is dense filtered TLAS order. Vulkan pipeline lookup uses instance custom index; Vulkan query lookup uses instance ordinal; Metal uses its instance ordinal. Supply the dense ordinal as Vulkan custom index too and keep table order identical on both backends. Check the 24-bit Vulkan custom-index capacity before GPU submission; never depend on masking/truncation. Canonical frame objectId is correspondence only. Persistent nodeId/submesh pairs remain CPU mapping keys, with duplicate identity diagnosed when temporal correspondence is needed. M02 pick IDs, selectable IDs and catalog generations never address these tables.

Interpolate both UV sets/normals/tangents using barycentrics (1-u-v,u,v). Transform positions by model, normals by inverse transpose; reject singular/nonfinite transforms and malformed triangles explicitly. Establish tangent handedness under mirrored transforms. Use geometric normals for bias and GI bounces; reflections may use the material normal map with a defined tangent basis. No analytic-shape shortcut or fake camera projection for UV-space tracing.

Memory is O(64*V + 4*I + 160*N + 256*M) plus descriptors, TLAS/BLAS and effect images. Packing duplicates mesh data already on the GPU, in exchange for avoiding native pointer/buffer-address ABI and Metal pointer residency complexity. Build immutable geometry at a stopped edit boundary and reuse it; acquired-slot dynamic instances/material snapshots must remain immutable through completion. Do not repack all vertices every frame.

## Material resources: alternatives and recommended limits

**Superseded capacity policy:** the profile and throwing-overflow proposal below
records the original M03 checkpoint. The later lead-directed [M07 correction](../milestone_07_bg2e_composer/tracing-capacity-16384.md)
uses exact device-clamped counts up to 16384, Vulkan specialization ID0, Metal
argument-record buffers8/9 and explicit warning/neutral-texture overflow. Object
materials remain independently owned/importable/exportable. Do not reinstate the
old profiles or strict table-overflow rejection from this historical proposal.

| Option | Complexity / fidelity | Recommendation |
|---|---|---|
| Extend static switch over image/sampler pairs | Small implementation, grows shader code and consumes native slots; arbitrary scenes repeatedly hit a small fixed limit | Keep existing shadows only; not the advanced ABI |
| Texture atlas or 2D-array packing | Requires resampling/format/dimension/mip grouping and wrap/border handling; can alter authored filtering and sRGB semantics | Reject as default; not required for this milestone |
| Native vertex addresses + unrestricted bindless tables | Extra addressing/residency/feature/lifetime framework; unnecessary for current meshes | Reject for this implementation |
| Packed geometry + bounded descriptor/argument arrays | Moderate GPU API extension, retains original images, views and samplers, scales to device limits | Recommended, subject to lead acceptance |

Separate image and sampler tables deduplicate actual view/range and sampler identities. Each of six material slots supplies independent image and sampler indices. Include valid neutral fallback resources in the counts. Use a fixed declared capacity per shader/layout profile, selected at initialization from a small compiled profile set (proposed 32, 128, 256 entries per table); select the largest profile supported by the device and total stage/set limits after reserving environment/effect resources. These are proposed profile ceilings, not promised universal capacities. M03-02 must implement matching native shader specialization/variants and publish effective capacities; no partial binding, variable descriptor count or update-after-bind is needed. Populate every unused element with a valid fallback.

Vulkan requires enabled sampled-image/sampler nonuniform indexing features and nonuniformEXT at divergent accesses; fixed arrays avoid a runtimeDescriptorArray requirement. Metal requires checked Tier-2 argument-buffer support, sampler argument-buffer compatibility, native encoding and useResource declarations for indirectly referenced images/buffers. Scalar/direct existing bindings stay usable. GPU descriptions retain explicit Vulkan set/binding and Metal buffer/argument indices; no hidden backend-neutral shader generator. Proposed API additions are indexed image/sampler assignment and explicit array binding layout metadata, plus primitive capability/limit reports. Exact signatures belong to M03-02.

If no profile works, expose advanced textured tracing as unavailable while keeping raster and the existing shadow query path valid. If a supported profile overflows for a particular scene, throw a diagnostic with required/available image/sampler counts before replacing a working snapshot. Never truncate, substitute textures silently or claim arbitrary unlimited material support. Scene errors are distinct from unavailable hardware. Huge geometry similarly fails explicit buffer/count validation; paging is outside this milestone.

Native basis: [Vulkan nonuniform indexing feature requirements](https://docs.vulkan.org/spec/latest/chapters/features.html), [Apple argument-buffer arrays and residency](https://developer.apple.com/documentation/metal/managing-groups-of-resources-with-argument-buffers), and [Apple argument-buffer resource arrays](https://developer.apple.com/documentation/Metal/using-argument-buffers-with-resource-heaps). These justify capability/encoding requirements; they do not prove support on the lead's hardware or in the current wrapper.

## Shading, cutouts and transparency

Reuse MaterialData's six slots, per-slot UV selection/scale, color-space mask, scalar channels/inversion, albedo alpha/cutoff, emission, unlit, roughness, metalness, Fresnel and sheen meanings. Preserve original images/views/samplers and avoid double sRGB conversion; alpha remains linear. Ray sampling uses explicit LOD0 initially, matching query-cutout behavior; derivative/ray-cone LOD is deferred and is not implied by material mip preservation. GI keeps production geometric/interpolated normals; do not require six texture samples for each visibility ray.

Opaque/cutout meshes are the default tracing participants. Per-instance ForceNonOpaque exposes cutout candidates even with opaque BLAS; the acceptance predicate is sampled albedo alpha times material alpha >= cutoff, after the correct UV selection/scale. Rejection must continue to geometry behind the hole in every visibility and nearest-hit path. Material edits changing cutoff/opacity rebuild the instance/material snapshot coherently. Never force opaque at ray level when alpha testing is needed.

Both blended transparent classes remain excluded as occluders and secondary-hit geometry by default, as in the current collector. They may receive effects on their own raster surface. Colored transmission, stochastic alpha blending, multi-layer refraction and transparent secondary transport are deferred; acceptance of this checkpoint explicitly accepts that boundary, not a claim that deferred raster transparency is fixed. Do not turn blended materials into opaque trace hits without a new lead decision.

## Public effect, surface and history contracts

Public passes accept borrowed invocation inputs plus retained compatible configuration/shaders, and report requested settings separately from effective availability/status. Common tracing resources, lights and environment snapshots are shared; mutable output/history ownership belongs to each effect/view/layer. No concrete RendererDeferred private graph access is required.

Surface input profiles describe extent, world position or depth plus inverse view-projection, geometric/shading normal, material/coverage and optional motion/identity. Every profile specifies format, coordinate space, valid-pixel rule, sampling and lifetime. Camera surfaces reconstruct positions; future bake surfaces supply explicit UV-space world-position/normal/coverage images. Bake never fabricates a camera or borrows picking/analyzer data.

AO outputs scalar visibility with neutral 1 and composes without discarding material AO. GI outputs linear HDR indirect lighting with explicit coverage/validity; unavailable/disabled GI selects ordinary IBL/baked behavior. Reflections output linear HDR radiance plus confidence/validity; a miss can sample environment, while unavailable/rejected results preserve specular IBL. M03-04–06 must specify whether each signal replaces or weights its corresponding ambient term and match production normalization deliberately; black alone is not an adequate availability signal. AO and GI mode selection remains mutually coherent; do not multiply ambient attenuation twice. GI/AO quality extents follow declared internal surface scaling; reflections quality is relative to display output independently of FSR. Supply explicit extents for non-camera callers.

M03-07 adds a public renderer/pass invalidation contract (proposed `invalidateHistory(reason)`), separate from resetUpscaler. At the queued pre-acquire drained edit boundary, material/texture/light/environment edits invalidate tracing snapshot revisions and affected effect histories; topology or scene replacement rebuilds geometry/TLAS and clears object correspondence. Camera cuts, resize, quality, shader/pass replacement and RT mode changes reset the appropriate view/layer histories. Dynamic transforms update slot-safe instances and require previous-object transforms for reprojection. Track tracing scene revision, rendered-view sequence and history validity separately from frame-in-flight slot and presentation counters. Paused/overlay/pick-only frames are not new scene samples. FSR reset is coordinated when needed but never substitutes for effect reset. AO, GI and reflection histories remain independent; bake and offscreen exact averages are separate accumulators.

## Dependency handoff and verification

M03-02 implements only accepted GPU prerequisites plus a primitive arbitrary-mesh/cutout probe, not full scene/effect policy. M03-04 establishes the public tracing snapshot/surface contracts reused by GI/reflections; their CPU table builder belongs in draw::pipeline. M03-03 supplies independent BlueNoise. M03-07/08 own histories and filtering. M03-09 integrates existing examples; M03-10 audits actual limits and updates bake consumers. M04/M08 reuse the snapshot/material/candidate and surface contracts without a camera renderer or their own shading-table family.

Lead validation for this step is design review only: select the recommended addressing/backend/capacity/transparency/reset policy or request changes. No new executable, header ABI or shader is delivered. The agent inspected the listed sources, compared native documented requirements, checked scoped Markdown links and ran git diff --check; no build, shader compilation, GPU dispatch or runtime check was performed.

M03-02 lead checks must cover arbitrary indexed multi-submesh geometry, UV0/UV1, nonuniform and mirrored transforms, cutout rejection revealing the rear hit, more than eight textures, distinct samplers for one image, all six material slots, noncoherent writes, slot retirement, profile overflow and unsupported indexing. Exercise Vulkan pipeline and query independently, Metal nearest-hit compute, raster/no-RT fallback and the existing GPU RT example. Later effects add image-quality/temporal/edit checks. M02 validation does not establish these advanced requirements.

## M03-10 actual implementation reconciliation

The [technical review](review.md) maps this accepted design to current source.
Packed 64/160/256-byte records, full 32/128/256 profiles, native Vulkan hit stages
and Metal compute queries are implemented. Reflection alpha is fractional accepted
geometry hits with IBL fallback, GI alpha is binary validity and RGB is incoming
diffuse radiance/pi. Display histories are bounded/reprojected and denoise lacks
chart IDs; exact bake/scene averaging and chart-safe filtering remain concrete
M04/M05 work. Legacy eight-cutout query shadows now degrade the whole scene to
neutral visibility with a diagnostic rather than failing the interactive renderer;
advanced capacities still fail explicitly and never truncate. Manipulation alone
starts GI/reflections enabled and exposes local light poses. No new architecture
decision or full secondary PBR/transparent transport parity is claimed.
