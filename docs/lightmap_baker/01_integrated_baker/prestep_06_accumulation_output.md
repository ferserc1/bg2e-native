# Handoff for Add accumulation and CPU/GPU results

Next implementation step: [step-06_accumulation_output.md](step-06_accumulation_output.md). Step 05 is complete; the project lead manually verified the fixture as passing. Step 06 is ready to begin.

- Changed files and relevant API decisions: Added UV-space RTGI and RGB composition for RTAO/RTGI. Preserved screen-space APIs. `Renderer::environmentResources()` exposes renderer-owned IBL resources through non-owning injection into `IntegratedBakerContext`; the renderer must outlive the context and its bakers. Baked AO textures are sampled as RGB, with direct lighting and emission kept outside the multiplier. The lightmap settings comments document RGB RTAO visibility and RTGI irradiance/reference normalization.
- Build command, platform and result: `cmake --build build --target test_03_integrated_baker_context` succeeded on Linux/Ninja.
- Runtime/fixture evidence: The project lead manually ran and verified the integrated baker fixture as passing for RTAO, RTGI and RGB lightmap output.
- Remaining limitations or regressions: Step 06 remains unimplemented. Follow the existing API contract and native-resolution requirements below.
- Resources, ownership and synchronization cautions for the next agent: Environment resources are renderer-owned and injected non-owningly; keep the renderer alive until all contexts and bakers are destroyed. Do not explicitly reset or delete `EnvironmentResources` before `Engine::cleanup()`; leave its owning pointer alive through engine cleanup and allow automatic destruction afterward. Vulkan resources are ordered by the engine's cleanup manager, so early explicit destruction can reverse the intended teardown order and trigger callbacks that invoke methods on already-destroyed objects. Apply this generally to objects whose Vulkan resources are managed through the engine cleanup manager. Continue using the context-owned TLAS and its per-frame-slot descriptor allocators. Register descriptor pool requirements before initializing each pool, and do not clear slot resources before its fence completes.
- Exact next action: Begin implementing the scope in `step-06_accumulation_output.md`. Have the project lead run the tests manually and wait for their verification before marking step 06 complete.

## Next-step instructions

Add per-baker progressive accumulation over configurable frames. Callers reset history before baking again after scene or target changes; settings are fixed when a baker is constructed, so changing them requires a new baker. Use full-resolution intermediate images. Offer configurable RGB8 or RGB32F CPU buffers, with RGB8 as the default, and optional sampled/renderable Vulkan image access. Ensure GPU-to-CPU synchronization and destructor cleanup. Assess FSR NativeAA compatibility with UV-space data; use native-resolution accumulation when it cannot apply without camera reprojection.

## Next-step acceptance gate

A baker can update repeatedly, read an RGB result, expose its image, and release resources safely. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk. The project lead runs tests manually; wait for verification before marking the step complete.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Add a per-baker `UvTemporalAccumulator` with two full-resolution float history images, the UV valid-texel mask and progressive sample count. Index history by UV texel; do not use camera view/projection or motion vectors. `resetAccumulation()` clears logical history; callers invoke it after geometry reload or scene changes before another bake sequence. Exactly `accumulationFrames` updates complete a bake; extra updates fail until reset.
- Average only samples of the same UV texel. Invalid texels receive neutral white. No island identifier, spatial denoising or padding dilation is required in this step; atlas seam filtering and padding fill are left outside this plan. Keep GPU accumulation in linear float and quantize only at RGB8 readback.
- `LightmapBaker::readPixels()` is called after submission and frame advancement, synchronizes with the last update, reports an error while still on its recording frame, and returns tightly packed top-row-first RGB8 or RGB32F as configured. Use transfer-capable images and a staging buffer. `image()` returns a shared image in shader-read layout with sampled/color-attachment usage.
- Test destruction while a submitted frame is still in flight and use the engine deferred cleanup facility or wait for the owning fence. This covers both per-baker resources and the context-owned per-slot `RayTracingScene`; `FrameResources::flushFrameData()` must not release bake TLAS resources. No use-after-free of descriptors, images or buffers is permitted.
- Assess FSR NativeAA at native atlas resolution. If valid UV-space motion/depth inputs cannot be supplied, treat it as unavailable for the baker and use native-resolution per-texel accumulation. Record the capability decision and reason in the implementation handoff; never silently scale any intermediate layer.
