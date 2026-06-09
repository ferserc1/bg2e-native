# Step 015 — Example `05_simple_triangle`: create pipeline, bind, `draw(3)`

## Objective

Make `05_simple_triangle` render a triangle: load the backend-appropriate shader modules, create a
`gpu::PipelineLayout` and `gpu::GraphicsPipeline`, and in the frame loop bind the pipeline and call
`draw(3)` between `beginRendering` and `endRendering`. **First visual milestone, both backends.**

## Context

All API pieces exist: shader modules (004/005), pipeline layout (006–008), graphics pipeline
(009–011), `bindPipeline` (012), `draw` (013). The example ships GLSL+MSL (014). The example already
branches on backend type for window/instance selection — reuse that branch to pick the shader files.

## Expected prior state

- Step 014 done (example builds, ships shaders, runs as clear loop).

## Files expected to review / modify

- Modify: `examples/gpu/05_simple_triangle/src/main.cpp`.
- Review: bundled shader path resolution; `gpu::Device` factory methods; `SurfaceFrame::colorImage()`
  / `depthImage()` for attachment formats.

## Proposed design

- After device creation, **before** the loop:
  - Resolve shader paths per backend:
    - Vulkan: `triangle.vert.spv`, `triangle.frag.spv`, entry point `"main"`.
    - Metal: `triangle.metallib`, entry points `"triangle_vertex"` / `"triangle_fragment"`.
  - `auto vs = device->createShaderModule({ vsPath, vsEntry, ShaderStage::Vertex });`
    `auto fs = device->createShaderModule({ fsPath, fsEntry, ShaderStage::Fragment });`
  - `auto layout = device->createPipelineLayout({});` (no push constants yet).
  - Read attachment formats from a frame (or from the surface's known formats —
    `B8G8R8A8_UNORM` color, `D32_SFLOAT` depth as configured in `createWindowSurface`). Prefer reading
    `frame->colorImage()->pixelFormat()` once to stay correct.
  - `GraphicsPipelineDescription desc{ vs.get(), fs.get(), layout.get(), TriangleList, colorFormat, depthFormat };`
  - `auto pipeline = device->createGraphicsPipeline(desc);`
  - Keep `vs`, `fs`, `layout`, `pipeline` alive for the whole loop (non-owning pointers in the pipeline).
- In the loop, inside the render scope:
  ```
  cmd->beginRendering(frame.get());
  cmd->clearColor(0, clearColor);
  cmd->clearDepth(1.0f);
  cmd->bindPipeline(pipeline.get());
  cmd->draw(3);
  cmd->endRendering();
  ```
- Cleanup: destroy pipeline, layout, shader modules before `device->cleanup()`.

Decision point — **format source:** color/depth formats must match both the pipeline and the frame.
Read them from the first `SurfaceFrame`'s images (authoritative) rather than hardcoding.

## Required changes (example only)

- Pipeline creation + per-frame bind/draw + cleanup in `main.cpp`. No engine changes.

## Compilation criteria

- Build green on all platforms; example links against the new `gpu` API only.

## Validation criteria

- Running `gpu_simple_triangle` and selecting **Vulkan** shows a triangle over the animated clear.
- On macOS selecting **Metal** shows the same triangle.
- No Vulkan validation errors; no Metal pipeline/encoder errors.

## Risks / points to check

- **Format mismatch** (pipeline vs frame) is the most likely failure on both backends — read formats
  from the frame.
- **Resize:** pipeline does not depend on size (dynamic viewport on Vulkan; Metal viewport defaults to
  the render target). Confirm resize still works (recreate not required).
- Metal encoder lifetime (step 012) must be correct, else the draw is dropped or validation fails.
- Shader path resolution differences between platforms / bundle layouts.

## What must NOT be done in this step

- No vertex buffers, uniforms, textures, push constants (push constants come in 021).
- No compute yet (016–020).
