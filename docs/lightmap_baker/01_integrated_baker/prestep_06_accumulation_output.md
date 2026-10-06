# Handoff for Add accumulation and CPU/GPU results

Next implementation step: [step-06_accumulation_output.md](step-06_accumulation_output.md). Complete this file **after** finishing step 05; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in step-06_accumulation_output.md.

## Next-step instructions

Add per-baker progressive accumulation over configurable frames; reset history after scene, target or parameter invalidation. Use full-resolution intermediate images and filtering. Offer configurable RGB8 or RGB32F CPU buffers, with RGB8 as the default, and optional sampled/renderable Vulkan image access. Ensure GPU-to-CPU synchronization and destructor cleanup. Assess FSR NativeAA compatibility with UV-space data; use a native-resolution path when it cannot apply without camera reprojection.

## Next-step acceptance gate

A baker can update repeatedly, read an RGB result, expose its image, and release resources safely. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Add a per-baker `UvTemporalAccumulator` with two full-resolution float history images, a valid-texel/identity mask and progressive sample count. Index history by UV texel; do not use camera view/projection or motion vectors. Reset on `resetAccumulation()`, settings changes, geometry reload or scene generation changes. Exactly `accumulationFrames` updates complete a bake; extra updates fail until reset.
- Apply edge-aware denoise only within the same island/surface ID, then dilate valid results into xatlas padding without crossing island boundaries. Denoise/coverage operations remain full resolution. Keep GPU accumulation in linear float and quantize only at RGB8 readback.
- `LightmapBaker::readPixels()` synchronizes with the last submitted update, reports an error if invoked before submission, and returns tightly packed top-row-first RGB8 or RGB32F as configured. Use transfer-capable images and a staging buffer. `image()` returns a shared image with documented layout and sampled/color-attachment usage.
- Test destruction while a submitted frame is still in flight and use the engine deferred cleanup facility or wait for the owning fence. This covers both per-baker resources and the context-owned per-slot `RayTracingScene`; `FrameResources::flushFrameData()` must not release bake TLAS resources. No use-after-free of descriptors, images or buffers is permitted.
- Attempt FSR NativeAA at native atlas resolution when valid UV-space motion/depth inputs can be supplied. If they cannot, treat it as unavailable for the baker and use native-resolution filtering. Record the capability decision and reason in the implementation handoff; never silently scale any intermediate layer.
