# M03-02 — GPU tracing stages and shader data access selected by checkpoint

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M03-01 accepted design; M01 query primitives.

## Scope and deliverable

- `lib/include/bg2e/gpu/Common.hpp`
- `lib/include/bg2e/gpu/RayTracingPipeline.hpp`
- `lib/include/bg2e/gpu/ResourceSet.hpp`
- `lib/include/bg2e/gpu/ShaderLib.hpp`
- `lib/src/bg2e/gpu/vk/RayTracingPipeline.cpp`
- `lib/src/bg2e/gpu/metal/RayTracingPipeline.cpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/gpu/Common.hpp](../../../../lib/include/bg2e/gpu/Common.hpp), [lib/include/bg2e/gpu/RayTracingPipeline.hpp](../../../../lib/include/bg2e/gpu/RayTracingPipeline.hpp), [lib/include/bg2e/gpu/ResourceSet.hpp](../../../../lib/include/bg2e/gpu/ResourceSet.hpp), [lib/include/bg2e/gpu/ShaderLib.hpp](../../../../lib/include/bg2e/gpu/ShaderLib.hpp), [lib/src/bg2e/gpu/vk/RayTracingPipeline.cpp](../../../../lib/src/bg2e/gpu/vk/RayTracingPipeline.cpp), [lib/src/bg2e/gpu/metal/RayTracingPipeline.cpp](../../../../lib/src/bg2e/gpu/metal/RayTracingPipeline.cpp).

## Implementation work

1. Start from the already implemented RayTracingMeshDescription::opaque and RayTracingOpacity Geometry/ForceOpaque/ForceNonOpaque mappings on both backends. Implement only additional checkpoint-required Vulkan any-hit/groups/stage visibility, resource arrays or buffer ranges/addressing and native handlers. Do not reimplement M01 query opacity primitives.
2. Preserve explicit per-backend ShaderBinding indices and add matching ShaderLib stage loading only when actual shaders need it; Metal uses separate equivalent kernels and nullable stages as documented.
3. Validate material/geometry table capacity, primitive device/format/shader-stage requirements and acceleration build/read dependencies.
4. Keep scene traversal, material policy, effect settings and sampling algorithms in draw; GPU extensions remain callable with primitive resources alone.

## Verification and exit conditions

- A small primitive validation dispatch/candidate test demonstrates actual hit data/cutout behavior on supported backends and rejects incompatible resources.
- Existing query-soft-shadow and RT demonstration paths still work; unavailable advanced support is reported independently of the raster baseline.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M03-04–M03-06 and bake passes depend on these exact data-access/stage contracts; amend them if implementation differs from the checkpoint.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-09 actual extraction baseline

Reuse retained RenderQueue Items/CollectRayTracingInstancesVisitor metadata and ShaderData from [bindings-visitors.md](../milestone_01_deferred_renderer/bindings-visitors.md). Current bindings target raster vertex/fragment stages only; compute/RT material tables and temporal previous matrices must be implemented explicitly here without assuming the fragment resource set binds compute. Instance IDs are one-based frame IDs, not filtered-vector indices or stable temporal identities. Use stable nodeId/submesh keys and preserve native buffer/submesh ranges and six-texture color/UV/alpha metadata.

## M01-11 lookup and opacity primitives

The [actual shadow query table](../milestone_01_deferred_renderer/query-shadows.md) supplies dense TLAS-instance indexing and submesh-local firstIndex-resolved UV triples plus cutoff/albedo alpha. It is limited to eight distinct statically bound cutout image/sampler pairs and is not a complete ray shading table. Reuse generic geometry/instance opacity and canonical correspondence, then implement the arbitrary data path selected in M03-01. Vulkan and Metal expose non-opaque triangle candidates; preserve alpha rejection without a force-opaque ray override. Query visibility uses compute with no render-stage requirement; advanced tracing execution remains the separate native design checkpoint.

## M03-01 checkpoint handoff — accepted by lead continuation

Implement the accepted [M03-01 design](rt-design.md): AnyHit/stage visibility and native Vulkan single-group SBT hardening; scalar-compatible bounded indexed image/sampler bindings, nonuniform-indexing and argument-buffer capabilities/limits, Metal Tier-2 encoding/residency, and complete fallback-filled profiles. No raw geometry address framework or texture atlas. Validate optional-stage SBT offsets, base/record alignment, noncoherent flush, device ownership and failed creation cleanup. Preserve current scalar/query/demo callers. GPU receives primitive buffers/resources only; the public draw tracing snapshot builder is introduced with M03-04. The primitive probe must exercise packed indexed hit lookup, cutout continuation, divergent images/samplers, more than eight textures and explicit capacity failure on both native paths. The selected implemented signatures and profile contract are recorded in [rt-gpu-data.md](rt-gpu-data.md).

## Delivery

See [rt-gpu-data.md](rt-gpu-data.md) and the [manual primitive fixture](../../../../tests/gpu/advanced_tracing/README.md). GPU APIs, native implementations and 32/128/256 probe shaders are delivered; compilation/runtime validation remains with the project lead. M03-03 is the next step.
