# FrameContext

**Header:** `<bg2e/draw/FrameContext.hpp>`  
**Namespace:** `bg2e::draw`

An aggregate passed to the production scene delegate's update/render callbacks.

```cpp
struct FrameContext {
    Engine& engine;
    gpu::CommandBuffer& commandBuffer;
    gpu::Image& colorTarget;
    gpu::Size2D extent;
    uint64_t frameNumber = 0;
    uint32_t frameSlot = 0;
    float deltaSeconds = 0.0f;
    uint64_t sceneFrameNumber = 0;
    float sceneDeltaSeconds = 0.0f;
};
```

| Field | Contract |
|-------|----------|
| `engine` | Borrowed graphics context. |
| `commandBuffer` | Borrowed commands for this operation. |
| `colorTarget` | Retained scene color image; not necessarily the acquired presentation image. |
| `extent` | Target pixel dimensions. |
| `frameNumber` | Window presentation sequence or offscreen scene sequence. |
| `frameSlot` | Reusable in-flight resource slot; distinct from the accumulated frame number and swapchain image index. |
| `deltaSeconds` | Window elapsed seconds or fixed offscreen step. Both delta fields use seconds. |
| `sceneFrameNumber` | Zero-based scene submission sequence; paused/UI-only presentations do not advance it. |
| `sceneDeltaSeconds` | Active elapsed time since the previous submitted scene, excluding paused time. |

References are valid for the current operation only; do not store the context
for deferred work. A const FrameContext still contains mutable references to
GPU commands and target resources. Objects may use frameSlot to select their
own persistent resources. Surface::beginFrame completes previous use of the
slot before RenderLoop constructs the context. Unavailable drawables do not
advance the slot or accumulated frame number. In window mode the color target is persistent
scene storage rather than the current drawable; no depth target is supplied.

See [TemporalAccumulator](TemporalAccumulator.md) for independent rendered-view effect histories, borrowed public surfaces, camera motion, bounded convergence and invalidateHistories application notifications.

For [offscreen execution](../app/OffscreenApplication.md#draw-execution),
both frame counters use the zero-based scene sequence, advancing only after GPU
completion. The first delta is zero and later deltas use the configured fixed
step (default zero). The color target is the acquired offscreen color image;
no presentation/UI frame or helper readback advances scene chronology.

The default accumulating delegate reuses the last completed scene slot/serial
for its separate final-only submission. That submission performs no scene update
and adds no scene sample. `didRenderFrame` follows the final GPU completion on the
last sample. See [offscreen jobs](DefaultOffscreenApplicationDelegate.md).
