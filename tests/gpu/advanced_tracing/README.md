# M03-02 advanced tracing primitive probe

This is a header-only manual fixture, not an example target or a test run recorded by the agent. It needs an already initialized gpu::Device with stopped frame producers. The project lead compiles and runs it. No CMake changes are required.

Include `tests/gpu/advanced_tracing/probe.hpp` from a temporary local harness (repository root on the include path) and call:

```cpp
const bool executed = bg2e::gpu::tests::runAdvancedTracingProbe(
    *device, backend, deployedShaderRoot);
```

`deployedShaderRoot` is the actual runtime `shaders` directory, containing `draw/rt_probe/probe{32,128,256}.*`. Existing paired shader discovery builds/deploys these files. Call after device initialization, before active frame acquisition; keep Device alive until all fixture locals are destroyed. All fixture submissions/readbacks complete synchronously. A thrown exception is failure; false is unsupported/skip, and true means the supported paths executed with matching pixels. Record backend, device, profile count, query/pipeline coverage and native diagnostics yourself.

## Expected checks

- Vulkan compute queries run when querySupported; Vulkan native raygen/miss/closest-hit/any-hit additionally runs when pipelineSupported. Metal uses its separate compute/intersection-query kernel. No native Metal SBT pipeline is claimed.
- Two overlapping indexed triangle submeshes, nonzero firstIndex and separate packed vertex base must resolve the correct UV1 and material. Dense IDs 0/1 differ from test canonical IDs 41/93.
- Thirty divergent front image choices exceed the old eight-pair shadow table. Alternating alpha rejects the front triangle; a rear opaque triangle must then win. Odd front indices keep the near hit. Native candidate traversal must choose nearest committed hit, independent of traversal order.
- UV1 is 1.25 at the front, 0.25 at the rear; repeat versus clamp switches a two-texel image between red and green. UV0 is 0.25, so a wrong UV set/index/base is visible. Every output byte is compared with a tolerance of one.
- Wrong array lengths, null images/samplers, incomplete tables and oversized layouts throw. Vulkan also rejects a closest-hit module used as any-hit, and constructs a missing-miss pipeline to check optional-group SBT indexing (not dispatched).
- Re-run after completion and under native validation. Check wrong-device resources with a second initialized device, invalid storage-buffer usage/range, and all three profiles supported by the device. Array updates are allowed only after their users complete; this synchronous fixture does not verify slot scheduling.
- Run existing gpu/13_ray_tracing_pipeline, draw/10_deferred_renderer, draw/11_manipulation and soft-shadow fixtures as regression checks. A device without tracing or nonuniform array support must keep scalar/raster/query-shadow behavior valid; skipping this probe must not be reported as advanced tracing success.

No engine effects, scene traversal, UI, accumulation/history or bake execution are introduced. See [the implementation handoff](../../../docs/render_to_draw/plan/milestone_03_ray_tracing/rt-gpu-data.md).
