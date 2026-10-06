# Step 06: Add accumulation and CPU/GPU results

## Scope

Add per-baker progressive accumulation over configurable frames; reset history after scene, target or parameter invalidation. Use full-resolution intermediate images and filtering. Offer configurable RGB8 or RGB32F CPU buffers, with RGB8 as the default, and optional sampled/renderable Vulkan image access. Ensure GPU-to-CPU synchronization and destructor cleanup. Assess FSR NativeAA compatibility with UV-space data; use a native-resolution path when it cannot apply without camera reprojection.

## Acceptance and compile gate

A baker can update repeatedly, read an RGB result, expose its image, and release resources safely. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- Add a per-baker `UvTemporalAccumulator` with two full-resolution float history images, a valid-texel/identity mask and progressive sample count. Index history by UV texel; do not use camera view/projection or motion vectors. Reset on `resetAccumulation()`, settings changes, geometry reload or scene generation changes. Exactly `accumulationFrames` updates complete a bake; extra updates fail until reset.
- Apply edge-aware denoise only within the same island/surface ID, then dilate valid results into xatlas padding without crossing island boundaries. Denoise/coverage operations remain full resolution. Keep GPU accumulation in linear float and quantize only at RGB8 readback.
- `LightmapBaker::readPixels()` synchronizes with the last submitted update, reports an error if invoked before submission, and returns tightly packed top-row-first RGB8 or RGB32F as configured. Use transfer-capable images and a staging buffer. `image()` returns a shared image with documented layout and sampled/color-attachment usage.
- Test destruction while a submitted frame is still in flight and use the engine deferred cleanup facility or wait for the owning fence. This covers both per-baker resources and the context-owned per-slot `RayTracingScene`; `FrameResources::flushFrameData()` must not release bake TLAS resources. No use-after-free of descriptors, images or buffers is permitted.
- Attempt FSR NativeAA at native atlas resolution when valid UV-space motion/depth inputs can be supplied. If they cannot, treat it as unavailable for the baker and use native-resolution filtering. Record the capability decision and reason in the implementation handoff; never silently scale any intermediate layer.

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
