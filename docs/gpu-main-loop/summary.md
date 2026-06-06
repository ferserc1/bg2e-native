# Plan — `bg2e::gpu` render/present main loop

**Goal:** implement the public API exercised by the *minimal animated clear* design sketch:
acquire a render target from a `Surface`, record commands into a `CommandBuffer`, transition
images, begin/clear/end a dynamic-rendering pass, present, and submit explicitly through a
`Queue`. Two backends: **Vulkan** (`gpu::vk`) and **Metal** (`gpu::metal`).

This plan **only** adds or modifies elements of the `bg2e::gpu` namespace. Nothing outside
`bg2e::gpu` is touched: the rest of the engine keeps compiling and running unchanged, because the
`gpu` layer is still in design and has no external consumers yet. The initialization flow
(`Factory`, `Instance`, `PhysicalDevice`, `Device`, `Surface` creation) is already implemented
and is **not** modified — see `examples/gpu/02_device/src/main.cpp` for the existing,
unchanged setup sequence the loop builds on.

## Target API (from the design sketch)

```cpp
auto& graphicsQueue = device->graphicsQueue();      // const Queue&  (already exists)

while (running) {
    // ... SDL events ...
    gpu::Color clearColor { r, g, b, 1.0f };

    auto frame = surface->beginFrame();              // std::shared_ptr<gpu::SurfaceFrame>
    auto cmd   = graphicsQueue.createCommandBuffer();// std::shared_ptr<gpu::CommandBuffer>

    cmd->begin();
    cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
    cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);
    cmd->beginRendering(frame.get());
    cmd->clearColor(0, clearColor);
    cmd->clearDepth(1.0f);
    cmd->endRendering();
    cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);

    surface->present(cmd.get());                     // schedule presentation onto cmd
    cmd->end();
    graphicsQueue.submit(cmd.get());                 // submit (+ vk queue present)
    surface->endFrame(frame.get());
}
```

## Key design decisions (locked by the sketch)

1. **Not a 1:1 Vulkan wrapper.** Vulkan is one backend; Metal must not be forced to mimic Vulkan
   semantics. Image-layout transitions are an explicit, useful abstraction at this level even
   though Metal mostly treats them as state bookkeeping / no-ops.
2. **`transition(image, desiredLayout)`** — two-argument form. The `Image` stores its own current
   layout; the `CommandBuffer` reads it, emits a backend transition only if it differs, and
   updates it. `CommandBuffer` is a `friend` of `Image`.
3. **`Surface` always owns a depth buffer** (already true today: `Surface::depthImage()` exists).
   `SurfaceFrame::depthImage()` surfaces it per frame even if a pass ignores it.
4. **`beginRendering` uses dynamic rendering** (Vulkan 1.3 / `VK_KHR_dynamic_rendering`, already a
   hard requirement of `vk::Device`). No `VkRenderPass`/`VkFramebuffer` is ever exposed. Metal
   builds a per-frame `MTL::RenderPassDescriptor` + `MTL::RenderCommandEncoder`.
5. **Clear values are recorded as load-op clears**, not as separate clear commands. `clearColor`/
   `clearDepth` set the attachment clear values that `beginRendering` will apply via `loadOp`
   (Vulkan) / `setLoadAction(Clear)` (Metal). Because Metal needs the clear values *before* the
   encoder is created, the Metal backend **defers encoder creation** until `endRendering` (or
   first draw); `beginRendering` only configures the descriptor. See step 03 for the exact
   ordering contract.
6. **`Queue::submit()` stays explicit** (also needed later for `immediateSubmit()` helpers).
7. **`Surface::present(cmd)` receives the command buffer**: Metal schedules
   `presentDrawable()` on the `MTL::CommandBuffer` *before* `commit()`; Vulkan records on the
   command buffer which frame must be presented and performs `vkQueuePresentKHR` inside
   `submit()`.
8. **`createCommandBuffer()` / `submit()` are `const`** member functions, because the loop holds
   `const Queue&` (`device->graphicsQueue()` returns `const Queue&`, unchanged). Backends use
   `mutable` / handle-only state to honor this.

## New / modified `bg2e::gpu` elements

| Element | Kind | Step |
|---|---|---|
| `gpu::Color` | new struct (`Common.hpp`) | 01 |
| `gpu::ImageLayout` | new enum (`Common.hpp`) | 01 |
| `gpu::Image` current-layout tracking | modify (`Image.hpp`) | 01 |
| `gpu::SurfaceFrame` | new abstract interface | 02 |
| `gpu::vk::SurfaceFrame`, `gpu::metal::SurfaceFrame` | new concrete | 02 |
| `gpu::CommandBuffer` | new abstract interface | 03 |
| `gpu::vk::CommandBuffer`, `gpu::metal::CommandBuffer` | new concrete | 03 |
| `gpu::Queue::createCommandBuffer()`, `submit()` | modify (add pure virtuals) | 04 |
| `gpu::vk::Queue` command pool; `gpu::vk::Device` pool wiring | modify | 04 |
| `gpu::metal::Queue` impl | modify | 04 |
| `gpu::Surface::beginFrame/present/endFrame/cleanup` | modify (add pure virtuals) | 05 |
| frame sync (vk semaphores/fences), present coupling in `vk::Queue::submit` | modify | 05 |
| `examples/gpu/04_clear_loop` | **optional, outside scope** | 06 |
| `gpu/all.hpp` includes for the new headers | modify | each step |

## Phase ordering and the "always compiles" invariant

Each step is atomic and leaves `libbg2e` compiling on every platform. Dependencies flow strictly
forward; no step depends on a later one.

- **Step 01** adds pure value types + image state. Purely additive, nothing references them.
- **Step 02** defines `SurfaceFrame` (abstract + concrete data holders). Never instantiated yet.
- **Step 03** defines `CommandBuffer` (abstract + concrete). Uses `SurfaceFrame` (step 02). Never
  instantiated yet (no `Queue` creates it).
- **Step 04** wires `Queue::createCommandBuffer()` / `submit()` (plain submit, no present). Adding
  pure virtuals forces *both* `vk::Queue` and `metal::Queue` to implement them in the same step.
- **Step 05** wires `Surface::beginFrame/present/endFrame/cleanup` across **all four** surface
  classes (vk/metal × window/offscreen) plus the Vulkan present-coupled `submit()`. Adding pure
  virtuals to `Surface` forces all subclasses to implement them in the same step.
- **Step 06** (optional, *outside* `bg2e::gpu`) adds a runnable example to validate end-to-end.

The reason `Queue` (step 04) precedes `Surface` lifecycle (step 05): `submit()` needs the
command-buffer machinery to exist before it can be coupled to per-frame presentation/sync.

See each `step-NN-*.md` for the exact signatures, member additions and backend mappings.
