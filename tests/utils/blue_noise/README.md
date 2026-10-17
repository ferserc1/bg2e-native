# M03-03 BlueNoise manual checks

The project lead compiles/runs this header-only fixture; no test or shader execution is recorded by the implementing agent. Include `tests/utils/blue_noise/probe.hpp` from a temporary local harness with the repository root on the include path.

```cpp
bg2e::utils::tests::checkBlueNoiseCpu();
const bool executed = bg2e::utils::tests::checkBlueNoiseGpu(*device, backend, deployedShaderRoot);
```

The CPU check requires no graphics initialization. The GPU check needs an initialized, stopped gpu::Device and the actual runtime shaders root containing `draw/blue_noise/probe.comp.*` and `fallback.comp.*`. It performs synchronous submissions/readbacks. false means unsupported RGBA8 usage, not a passing dispatch; an exception means failure. No renderer, Engine, UI, RT hardware or new CMake target is required.

Verify on Linux Vulkan and macOS Metal:

- CPU metadata, raw bytes, negative-coordinate wrapping, logical layer stride, rank-bin centers, repeatability and [0,1) range with uint32 sequence overflow. Invalid metadata/byte counts must throw.
- Read every uploaded RGBA8 atlas byte and compare source channels and padding. Four embedded logical layers must survive as a 128x512 color 2D image with one mip; they are not physical array layers.
- Compute shaders must match CPU blue-noise and hash samples within one RGBA8 quantization value. Output dimensions cross tile boundaries; negative coordinates and near-overflow frame/sample keys exercise wrapping and logical layer selection. The fallback dispatch binds no noise image/sampler.
- Invalid replacement must preserve published resources/metadata. Valid replacement creates an independent snapshot. Copied image/sampler owners from the old snapshot remain usable after replacement and utility cleanup, and are used by the dispatch in this fixture.
- Repeat build/cleanup/renderer or effect replacement with native validation enabled. Keep every exported owner alive through GPU completion and destroy them before the borrowed Device. Try custom 1..4-channel, non-square and single-layer tiles for raw upload; 2D noise sampling specifically requires at least two channels.
- Build existing production callers as a coexistence check. No production BlueNoise/shader/data/generator file is changed by M03-03. Effects are introduced in subsequent steps, so no AO/GI/reflection visual change is expected yet.

The existing `scripts/validate_blue_noise.py` can be run by the lead for spatial diagnostics if the rank source changes. This step reuses its existing data unchanged and does not regenerate or spectrally revalidate it. See [implementation details](../../../docs/render_to_draw/plan/milestone_03_ray_tracing/blue-noise.md).
