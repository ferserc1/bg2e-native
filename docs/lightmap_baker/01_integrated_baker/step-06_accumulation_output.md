# Step 06: Add accumulation and CPU/GPU results

## Scope

Add per-baker progressive accumulation over configurable frames. Callers reset history before baking again after scene or target changes; settings are fixed when a baker is constructed, so changing them requires a new baker. Use full-resolution intermediate images. Offer configurable RGB8 or RGB32F CPU buffers, with RGB8 as the default, and optional sampled/renderable Vulkan image access. Ensure GPU-to-CPU synchronization and destructor cleanup. Assess FSR NativeAA compatibility with UV-space data; use native-resolution accumulation when it cannot apply without camera reprojection.

## Acceptance and compile gate

A baker can update repeatedly, read an RGB result, expose its image, and release resources safely. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- Add a per-baker `UvTemporalAccumulator` with two full-resolution float history images, the UV valid-texel mask and progressive sample count. Index history by UV texel; do not use camera view/projection or motion vectors. `resetAccumulation()` clears logical history; callers invoke it after geometry reload or scene changes before another bake sequence. Exactly `accumulationFrames` updates complete a bake; extra updates fail until reset.
- Average only samples of the same UV texel. Invalid texels receive neutral white. No island identifier, spatial denoising or padding dilation is required in this step; atlas seam filtering and padding fill are left outside this plan. Keep GPU accumulation in linear float and quantize only at RGB8 readback.
- `LightmapBaker::readPixels()` is called after submission and frame advancement, synchronizes with the last update, reports an error while still on its recording frame, and returns tightly packed top-row-first RGB8 or RGB32F as configured. Use transfer-capable images and a staging buffer. `image()` returns a shared image in shader-read layout with sampled/color-attachment usage.
- Test destruction while a submitted frame is still in flight and use the engine deferred cleanup facility or wait for the owning fence. This covers both per-baker resources and the context-owned per-slot `RayTracingScene`; `FrameResources::flushFrameData()` must not release bake TLAS resources. No use-after-free of descriptors, images or buffers is permitted.
- Assess FSR NativeAA at native atlas resolution. If valid UV-space motion/depth inputs cannot be supplied, treat it as unavailable for the baker and use native-resolution per-texel accumulation. Record the capability decision and reason in the implementation handoff; never silently scale any intermediate layer.

## API example

```cpp
for (uint32_t i = 0; i < settings.accumulationFrames; ++i) {
    context->prepareFrame(cmdForFrame(i), frameFor(i));
    baker->update(cmdForFrame(i), frameFor(i));
}
auto pixels = baker->readPixels(); // after the final frame submission
```

## Handoff

After this step, complete [prestep_07_application_integration.md](prestep_07_application_integration.md) for the next step (Integrate into model_edit and bg2e_composer).
