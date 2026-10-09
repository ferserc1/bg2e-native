# FrameContext

**Header:** `<bg2e/draw/FrameContext.hpp>`  
**Namespace:** `bg2e::draw`

An aggregate passed to the experimental scene delegate's update/render callbacks.

```cpp
struct FrameContext {
    Engine& engine;
    gpu::CommandBuffer& commandBuffer;
    gpu::Image& colorTarget;
    gpu::Size2D extent;
    uint64_t frameNumber = 0;
    uint32_t frameSlot = 0;
    float deltaSeconds = 0.0f;
};
```

| Field | Contract |
|-------|----------|
| `engine` | Borrowed graphics context. |
| `commandBuffer` | Borrowed commands for this operation. |
| `colorTarget` | Retained scene color image; not necessarily the acquired presentation image. |
| `extent` | Target pixel dimensions. |
| `frameNumber` | Accumulated frame number. |
| `frameSlot` | Reusable in-flight resource slot; distinct from the accumulated frame number and swapchain image index. |
| `deltaSeconds` | Elapsed time in seconds, render callbacks use milliseconds. |

References are valid for the current operation only; do not store the context
for deferred work. A const FrameContext still contains mutable references to
GPU commands and target resources. Objects may use frameSlot to select their
own persistent resources. Surface::beginFrame completes previous use of the
slot before RenderLoop constructs the context. Unavailable drawables do not
advance the slot or accumulated frame number. The color target is persistent
scene storage rather than the current drawable; no depth target is supplied.
