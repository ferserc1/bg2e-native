# Step 5: Validation and Testing

## File

No new files. This step describes validation procedures for the SMAA implementation.

## Build Verification

1. Build the project with CMake:
   ```sh
   cmake -S . -B build -G Ninja -DVULKAN_SDK=/path/to/vulkan/sdk
   cmake --build build
   ```

2. Verify all five `.comp.glsl` shaders compile to `.spv` without errors. Check `bin/{platform}/shaders/` for:
   - `smaa_edge_detection.comp.spv`
   - `smaa_blend_weight.comp.spv`
   - `smaa_neighborhood_blend.comp.spv`
   - `smaa_area_generate.comp.spv`
   - `smaa_search_generate.comp.spv`

3. Verify the C++ compilation succeeds with no errors in:
   - `lib/src/bg2e/render/deferred/SMAAProcessor.cpp`
   - `lib/src/bg2e/render/RendererDeferred.cpp`

## Runtime Validation

### Vulkan Validation Layers

Run with Vulkan validation layers enabled (`_debugLayers = true` in Engine). Watch for:

- **Descriptor set errors**: Ensure all bindings match between shader and descriptor set layout.
- **Image layout errors**: All transitions must be valid. The validation layer will report `SYNC-HAZARD` or `LAYOUT` errors if transitions are incorrect.
- **Pipeline barrier errors**: Ensure compute-to-compute barriers are correct between passes.
- **Push constant errors**: Ensure push constant size and stage flags match the pipeline layout.
- **Storage image format errors**: Ensure `image2D` format qualifiers match the `VkFormat` of the image.

### Expected Validation Warnings

- First-frame warnings about `VK_IMAGE_LAYOUT_UNDEFINED` → `GENERAL` transitions are expected for newly created images.
- If any `WRITE_AFTER_READ` or `READ_AFTER_WRITE` hazards appear, add explicit pipeline barriers.

### Visual Verification

1. **Before SMAA**: Run the renderer without SMAA integration. Capture a screenshot of a scene with diagonal edges (e.g., a cube or sphere).

2. **After SMAA**: Run with SMAA enabled. Compare:
   - Diagonal edges should appear smoother.
   - Thin geometry (wireframes, grid lines) should have fewer aliasing artifacts.
   - The overall image should not appear blurry (SMAA is edge-selective).
   - UI overlay should render correctly on top of the anti-aliased image.

3. **Edge detection visualization**: Temporarily modify the shader to output `edgesImage` directly to the output. You should see:
   - White pixels on detected edges.
   - Noisy but coherent edge patterns on diagonal surfaces.
   - No edges detected on flat surfaces or sky.

4. **Blend weight visualization**: Temporarily output `blendWeightsImage`. You should see:
   - Colored pixels near edges (weights > 0).
   - Black pixels in smooth regions (weights = 0).
   - The four channels (RGBA) should correspond to the four blend directions.

### Performance Verification

1. **Frame time**: Compare frame time with and without SMAA. Each compute pass adds one dispatch. On modern GPUs, the overhead should be < 1ms total for 1080p.

2. **Memory**: Verify that per-frame images are allocated correctly:
   - `edgesImage`: ~0.5 MB per frame at 1080p (R8G8, 2 bytes/pixel)
   - `blendWeightsImage`: ~8 MB per frame at 1080p (RGBA8, 4 bytes/pixel)
   - `outputImage`: ~8 MB per frame at 1080p (RGBA8, 4 bytes/pixel)
   - Total per frame: ~16.5 MB at 1080p
   - LUT textures: ~72 KB (negligible)

3. **Resize**: Resize the viewport and verify no crashes or visual artifacts. Per-frame images should be recreated at the new size.

## Common Issues

### Issue: Black output image

**Cause**: Image layout mismatch. The `outputImage` is in `GENERAL` but the copy expects `SHADER_READ_ONLY`.

**Fix**: Ensure the final transition in `process()` is `GENERAL → SHADER_READ_ONLY_OPTIMAL`.

### Issue: Validation error "Descriptor set not updated"

**Cause**: Descriptor set is allocated but not properly written.

**Fix**: Ensure `beginUpdate()` / `addImage()` / `endUpdate()` are called in sequence for each pass.

### Issue: No visible anti-aliasing

**Cause**: Edge detection threshold too high, or LUT textures not generated.

**Fix**:
- Check that `_lutsGenerated` is true after `build()`.
- Lower `SMAA_THRESHOLD` in `lib/smaa.glsl` (try 0.01 instead of 0.05).
- Visualize `edgesImage` to confirm edges are detected.

### Issue: Validation error "Image used with conflicting layout"

**Cause**: `colorImage` layout transition conflicts with the renderer's expected layout.

**Fix**: The SMAA integration transitions `colorImage` to `SHADER_READ_ONLY_OPTIMAL` before reading, then to `TRANSFER_DST_OPTIMAL` for the copy, then to `COLOR_ATTACHMENT_OPTIMAL` for the UI. Ensure these transitions match what `RenderLoop` expects.

### Issue: GPUProcess LUT generation fails

**Cause**: `immediateSubmit` called before the command pool is ready.

**Fix**: Ensure `build()` is called after `Engine::init()` completes and the graphics queue is available.

## Test Scenes

Recommended scenes for SMAA verification:

1. **Simple geometry**: A cube or sphere with sharp edges. SMAA should smooth diagonal edges.
2. **Grid/floor plane**: Fine grid lines should appear cleaner.
3. **Text rendering**: UI text (if rendered in the 3D scene) should be smoother.
4. **Transparent objects**: Glass or transparent materials should not be affected (SMAA runs after transparency).
5. **RTAO shadows**: AO edges should be smoothed if they produce luma changes.
