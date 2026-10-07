# Step 02: Build the UV-space surface pass

## Scope

Rasterize the complete target Drawable, including every submesh and its transform, into full-resolution UV2-space surface data (world position, normal, material/submesh identity and valid-texel mask). Reuse `GBufferManager` for the attachments through an additive depthless configuration; do not reuse its camera-space attachment profile or `DeferredLayer`'s images. Keep UV1 material sampling intact. Define atlas overlap and missing-UV2 rejection. Ensure texture padding and empty texels are distinguishable. Add shaders and Vulkan resources needed for this pass.

## Acceptance and compile gate

A diagnostic result can identify covered texels and submeshes on a known UV2 fixture; the default camera G-buffer still has its existing five color attachments and D32 depth; code compiles. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- Add `GBufferManager::Configuration { std::vector<VkFormat> colorFormats; VkFormat depthFormat = VK_FORMAT_UNDEFINED; }`, an additive constructor taking it, and `beginRender(VkCommandBuffer)` for a depthless pass. Preserve the existing constructor and `beginRender(VkCommandBuffer, bool)` exactly. Make `build`, `resize`, transitions, `beginRender` and `cleanup` handle absent depth; the default camera profile must still allocate its existing five color attachments and D32 depth. A configured instance reports its own formats through `formats()`, null through `depthImage()` and `VK_FORMAT_UNDEFINED` through `depthFormat()` when depthless.
- Add a private `render::UvSurfacePass` and dedicated shaders; each `LightmapBaker` owns a separate `GBufferManager` inside this pass for each in-flight slot (standalone uses slot 0). Its input is one standard `scene::Drawable`, including all submeshes and node/drawable/submesh transforms. It renders triangles with UV2 as clip-space XY: `2 * texCoord1 - 1`, with a documented Vulkan Y convention. Use `GBufferManager::formats()` to configure the UV graphics pipeline and disable depth testing/attachment.
- Configure four full-resolution color images in this order: world position `VK_FORMAT_R32G32B32A32_SFLOAT`, signed world normal `VK_FORMAT_R16G16B16A16_SFLOAT`, zero-based submesh/material ID `VK_FORMAT_R32_UINT`, and valid-texel mask `VK_FORMAT_R32_UINT` (0 empty, 1 covered). Check color-attachment and sampled-image format features before allocation. Clear all images to zero. Apply inverse-transpose normal transformation; keep UV1 available for source material sampling. Read IDs and mask with integer samplers/nearest filtering. Reject triangles outside the atlas or with near-zero UV area.
- Use submesh-specific draw ranges. After the pass, transition attachment writes to compute/ray-tracing reads. Fix `Image::cmdTransitionImage` so `imageBarrier.dstAccessMask` uses `TransitionInfo::dstAccessMask` instead of the current `dstStageMask`; verify the resulting synchronization. Wait for a slot's previous GPU use before rebuilding/resizing its manager. Do not retain its images after cleanup. Do not use the camera G-buffer's depth test to cull overlapping UV islands: overlap is a validation failure.
- Add a private CPU usable-UV2 validator now: finite coordinates, [0,1] bounds, nondegenerate mapped triangles, valid index/submesh ranges and no positive-area overlap except shared boundaries. `createBaker` must invoke it before allocating targets. Phase 2 promotes it to `geo::UvAtlasValidator`; phase 3 reuses that public validator for CLI skip decisions. The mere existence of `texCoord1` is insufficient because current importers may copy UV1 into it.
- A diagnostic readback/visual fixture must show the same coverage for two submeshes in one atlas and no light leaking through uncovered texels. The ordinary renderer remains unchanged at this step.

## API example

`UvSurfacePass::record(cmd, targetDrawable, targetNode.worldMatrix(), extent, frameSlot)` is private; it selects the target's configured `GBufferManager` for that slot. Only the additive `GBufferManager` configuration and depthless render overload are public.

## Handoff

After this step, complete [prestep_03_scene_bindings.md](prestep_03_scene_bindings.md) for the next step (Build the context-owned bake RT scene).
