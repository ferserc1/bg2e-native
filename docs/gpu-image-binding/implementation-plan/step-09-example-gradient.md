# Step 09 — Example: gradient compute shaders + storage-image binding (Milestone 1)

## Title
Make the compute shader write a gradient background into the frame color image,
with the triangle composited on top.

## Objective
Wire the full resource-binding API into `examples/gpu/05_simple_triangle`: a
compute pipeline whose layout has a `StorageImage`/Compute binding, a per-frame
`ResourceSet` pointing at `frame->colorImage()`, and a gradient compute shader for
both backends. **This is the first visible result and the end of Part 1.**

## Objective recap of frame flow
```
frame = surface->beginFrame()
cmd->begin()
cmd->transition(colorImage, General)
cmd->beginCompute()
  cmd->bindPipeline(gradientComputePipeline)
  cmd->bindResourceSet(gradientComputePipeline, 0, gradientSet[frameSlot])
  cmd->dispatch(ceil(w/16), ceil(h/16), 1)
cmd->endCompute()
cmd->transition(colorImage, ColorAttachment)
cmd->transition(depthImage, DepthAttachment)
cmd->beginRendering(frame)
  // NO clearColor (load preserves gradient); clearDepth(1.0) still ok
  cmd->bindPipeline(trianglePipeline)
  cmd->pushConstants(Fragment, ...)
  cmd->draw(3)
cmd->endRendering()
cmd->transition(colorImage, Present)
surface->present(cmd); cmd->end(); graphicsQueue.submit(cmd); surface->endFrame(frame)
```

## Context
Only the example changes (C++ + its own shaders). The engine API from Steps 01–08
is now exercised for the first time.

## Expected previous state
- Steps 01–08 complete. Color target is storage-capable; Metal preserves color on
  load; `bindResourceSet` works on both backends.

## Files to review / modify
- Review: `examples/gpu/05_simple_triangle/src/main.cpp` (current loop),
  `examples/gpu/05_simple_triangle/shaders/glsl/noop.comp.glsl`,
  `examples/gpu/05_simple_triangle/shaders/metal/triangle.metal`,
  `examples/gpu/05_simple_triangle/CMakeLists.txt` (shader build rules — no edit
  needed).
- Modify (example only):
  - Replace `shaders/glsl/noop.comp.glsl` with `shaders/glsl/gradient.comp.glsl`.
  - Add a gradient kernel to `shaders/metal/triangle.metal` (or a new `.metal`).
  - `src/main.cpp`: build compute layout with a `StorageImage` binding, create a
    ring of gradient `ResourceSet`s, rewire the loop, drop `clearColor`.

## Proposed design
### Shaders (example-only)
GLSL `gradient.comp.glsl`:
```glsl
#version 450
layout(local_size_x = 16, local_size_y = 16) in;
layout(set = 0, binding = 0, rgba8) uniform image2D outImage;
void main() {
    ivec2 p = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(outImage);
    if (p.x >= size.x || p.y >= size.y) return;
    vec2 uv = vec2(p) / vec2(size);
    imageStore(outImage, p, vec4(uv.x, uv.y, 0.5, 1.0));
}
```
Metal kernel (in `triangle.metal`, replacing `noop_compute`):
```metal
kernel void gradient_compute(texture2d<float, access::write> outImage [[texture(0)]],
                             uint2 gid [[thread_position_in_grid]])
{
    uint w = outImage.get_width(), h = outImage.get_height();
    if (gid.x >= w || gid.y >= h) return;
    float2 uv = float2(gid) / float2(w, h);
    outImage.write(float4(uv.x, uv.y, 0.5, 1.0), gid);
}
```
The `rgba8` storage format must match the chosen UNORM swapchain format (Step 08).

### C++ wiring (example-only)
1. Load the gradient compute shader instead of `noop` (`gradient.comp.spv` /
   `gradient_compute` Metal entry point).
2. Compute layout:
   ```cpp
   gpu::PipelineLayoutDescription computeLayoutDesc{};
   computeLayoutDesc.resourceBindings.push_back({
       /*set*/0, /*binding*/0, gpu::ResourceType::StorageImage, gpu::ShaderStage::Compute, 1 });
   auto computeLayout = device->createPipelineLayout(computeLayoutDesc);
   ```
3. Create a **ring** of resource sets, one per swapchain image, to avoid the
   in-flight update hazard:
   ```cpp
   std::vector<std::unique_ptr<gpu::ResourceSet>> gradientSets(surface->imageCount());
   for (auto& s : gradientSets) s = device->createResourceSet(computeLayout.get(), 0);
   uint32_t slot = 0;
   ```
4. Per frame, before dispatch:
   ```cpp
   auto* set = gradientSets[slot].get();
   set->setStorageImage(0, frame->colorImage());
   set->update();
   ...
   cmd->bindResourceSet(computePipeline.get(), 0, set);
   uint32_t gx = (w + 15) / 16, gy = (h + 15) / 16;
   cmd->dispatch(gx, gy, 1);
   ...
   slot = (slot + 1) % gradientSets.size();
   ```
   (`w`/`h` from `frame->colorImage()->width()/height()`.)
5. Remove the `cmd->clearColor(0, clearColor)` call so the render pass loads the
   gradient. Keep `clearDepth(1.0f)`.
6. Cleanup: release the resource sets and `computeLayout` (now non-empty) at the
   end, in addition to the existing cleanup.

## Compilation criteria
- Example compiles on both backends. The GLSL and Metal shaders build through the
  existing `CMakeLists.txt` rules (auto-discovered; no CMake edits).

## Validation criteria
- Window shows a smooth red/green gradient background with the RGB triangle drawn
  on top (triangle no longer sits on a flat clear color).
- Resizing the window keeps the gradient correct (ring of sets re-targets the new
  swapchain images; the surface recreates them on resize).
- No Vulkan validation errors (correct layouts: `General` for the storage write,
  `ColorAttachment` for the load+draw, `Present` for present).
- On Metal, no encoder/usage validation errors.

## Risks / things to check
- **Storage image format vs shader format qualifier:** the GLSL `rgba8` qualifier
  and the Metal write format must match the UNORM swapchain format from Step 08.
  A `bgra8` swapchain still presents correctly; `imageStore` writes component
  order per the view — verify colors are not swizzled (swap R/B in shader if the
  swapchain is BGRA and colors look wrong).
- **Ring sizing:** `surface->imageCount()` on Metal returns 3; on Vulkan it equals
  the swapchain image count. Sizing the ring to `imageCount()` and cycling avoids
  updating a descriptor set still in flight (Step 06 risk).
- **Dispatch group size vs Metal threads-per-group:** the current Metal
  `dispatch` uses `threadExecutionWidth` for threadsPerGroup width and height 1.
  For a 2D gradient this under-dispatches in Y. The example may need the Metal
  `dispatch` to use a 2D threadgroup. If `CommandBuffer::dispatch` cannot express
  a 2D threadgroup adequately, either (a) accept a 1-wide column per group
  (correct but slow), or (b) treat `groupCountX/Y/Z` as threadgroup counts with a
  fixed 16×16 threads-per-group. **Recommended:** confirm the Metal `dispatch`
  maps `dispatchThreadgroups(groups, threadsPerGroup)` with a sensible 2D
  `threadsPerGroup`; if not, this is a small, in-namespace adjustment to
  `metal::CommandBuffer::dispatch` (allowed — it is `bg2e::gpu`). Document the
  decision and keep GLSL/MSL local sizes (16×16) consistent.
- **No clear vs validation:** with `LOAD_OP_LOAD`, the color image must be in a
  defined state before the render pass — it is, because compute wrote every pixel.

## What NOT to do in this step
- Do not add the sampled texture to the triangle yet (Part 2).
- Do not add shaders to the engine library.
- Do not implement the offscreen fallback unless Step 08 proved storage on the
  swapchain is unsupported on the test hardware.
