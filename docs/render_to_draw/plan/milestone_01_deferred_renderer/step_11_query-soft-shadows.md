# M01-11 — Alpha-aware query soft shadows without RT pipelines

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M01-02/M01-09/M01-10.

## Scope and deliverable

- `lib/include/bg2e/gpu/RayTracingMesh.hpp`
- `lib/include/bg2e/gpu/RayTracingScene.hpp`
- `lib/src/bg2e/gpu/vk/RayTracingMesh.cpp`
- `examples/draw/07_ray_query_shadows/shaders/shadow.frag.metal`
- `shaders/src/glsl/lib/rt_material_data.glsl`
- `lib/include/bg2e/draw/pipeline/SoftShadowPass.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/gpu/RayTracingMesh.hpp](../../../../lib/include/bg2e/gpu/RayTracingMesh.hpp), [lib/include/bg2e/gpu/RayTracingScene.hpp](../../../../lib/include/bg2e/gpu/RayTracingScene.hpp), [lib/src/bg2e/gpu/vk/RayTracingMesh.cpp](../../../../lib/src/bg2e/gpu/vk/RayTracingMesh.cpp), [examples/draw/07_ray_query_shadows/shaders/shadow.frag.metal](../../../../examples/draw/07_ray_query_shadows/shaders/shadow.frag.metal), [shaders/src/glsl/lib/rt_material_data.glsl](../../../../shaders/src/glsl/lib/rt_material_data.glsl).

## Implementation work

1. Add primitive non-opaque geometry/instance flags and required query resource access on both backends. Build BLAS/TLAS through GPU commands; keep the high-level collector/material matching in draw.
2. Choose the smallest explicit geometry/UV/albedo-alpha table mechanism sufficient for cutout-aware queries, extending generic binding arrays/ranges if needed. Do not settle advanced RT pipeline architecture here.
3. Implement backend-specific equivalent soft-shadow kernels using per-light castShadows/sourceSize/shadowSamples and accepted triangle candidates. Transparent-hit filtering follows current behavior unless explicitly revised.
4. Gate query shadows using actual GPU query/acceleration/format capabilities; preserve unshadowed PBR fallback. No any-hit tracing pipeline, AO/GI/reflection or denoise allocation in this step.

## Verification and exit conditions

- A non-opaque cutout casts shadows through solid texels and not holes; source-size/sample changes alter softness on supported Vulkan/Metal paths.
- No-RT or forced-query-disabled rendering remains functional without BLAS/TLAS allocation requirements; no frame-by-frame global idle is introduced.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M03-01 must review the actual query table/cutout contract before deciding advanced tracing addressing and pipeline extensions.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-02 capability handoff

Use physical `RayTracingCapabilities::querySupported()` and logical `Device::rayTracingCapabilities()` rather than requiring `fullSupported()`. Check `rayQueryInRenderStages` before selecting a raster-stage intersection shader on Metal; compute-capable intersection does not establish raster-stage support. Preserve the planned supported shadow path/fallback when that capability is absent.

## M01-04 shader deployment handoff

Use the paired source/output paths and actual ShaderLib entry points documented in [shader-libraries.md](shader-libraries.md). The registered recursive draw shader tree already handles shared includes, native compilation and app/install/standalone deployment. Add concrete pipeline stages there; do not flatten outputs or treat the `probe` libraries as the final renderer implementation. Keep Vulkan `main`, Metal stage-specific entry points and explicit backend resource indices.

## M01-09 query identity and material handoff

Use CollectRayTracingInstancesVisitor on the same RenderQueue used for raster, preserving nodeId/submeshIndex, one-based 24-bit frame object ID, world matrix and retained mesh/BLAS/texture owners. IDs are dense traversal IDs and can change between frames; the per-instance GPU material/geometry table must map instanceId to its actual snapshot (do not assume filtered instance order equals objectId-1 without an explicit mapping). Both transparent classes are excluded by default, with explicit opt-ins; opaque alpha-tested materials are retained. Implement the real alpha-aware hit-table/UV sampling here, including firstIndex and the actual packed cutoff/UV/scale/channel/color semantics. Retain collections/TLAS buffers per submitted slot, skip unsupported/empty scenes and never silently truncate. See [bindings-visitors.md](bindings-visitors.md).

The current native BLAS implementations still mark all triangles opaque (Vulkan VK_GEOMETRY_OPAQUE_BIT_KHR and Metal setOpaque(true)). Implement/verify an appropriate generic non-opaque or force-non-opaque traversal path before claiming alpha candidate rejection works; CPU material extraction alone does not change that behavior. Preserve opaque fast paths where compatible and validate candidate cutout behavior on both backends.

## M01-10 raster integration

Extend the actual public DeferredLayer/deferredBindingLayout and paired draw/raster/deferred fragment stages described in [raster-layers.md](raster-layers.md). Fullscreen set1 is G-buffer/background, set2 lights, set3 environment (Metal textures8..11), set0 frame aliases. G-buffer geometry retains M01-09 material indices. Transparent lighting runs once per sorted item with working depth copied from opaque depth; no query placeholder object exists. Use the retained canonical queue and opaque-depth semantics, and preserve no-RT output. Native exact depth copies now support D16/D32 only. Keep query inputs/sets/mutable data alive per acquired slot/invocation without overwriting previous transparent calls.

## M01-11 implementation handoff

Actual public pass/settings, native opacity, dense UV/alpha tables, bindings, fallback and lead checks: [query-shadows.md](query-shadows.md). SoftShadowPass records compute ray queries on both backends; no render-stage query or tracing pipeline is required. The query-aware deferred variant multiplies only matching direct light. Eight distinct cutout image/sampler pairs are supported with an explicit ninth-pair error; this deliberately small table does not decide M03 advanced addressing. Native geometry/instance opacity controls preserve fast opaque paths and expose cutout candidates. No CMake changes or agent build/runtime execution.
