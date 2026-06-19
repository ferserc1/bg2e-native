# Step 01: Surface Frame Counter and inFlightFrames

## Files to Modify

- `lib/include/bg2e/gpu/Surface.hpp`

## Interface Changes

Add three new members to the `Surface` abstract class:

```cpp
class BG2E_API Surface {
public:
    // ... existing API unchanged ...

    // Number of swapchain images (existing, stays as-is)
    virtual uint32_t imageCount() const = 0;

    // NEW: Number of frames that can be in flight concurrently.
    // Window surfaces return 2, offscreen surfaces return 1.
    // This is distinct from imageCount() — the swapchain may have
    // more images than frames in flight.
    virtual uint32_t inFlightFrames() const = 0;

    // NEW: Monotonically increasing frame counter.
    // Incremented automatically by endFrame() in concrete surfaces.
    // Used by CleanupManager to index deferred cleanup closures.
    uint64_t frameCounter() const { return _frameCounter; }

    // ... existing frame lifecycle ...
    virtual std::shared_ptr<SurfaceFrame> beginFrame() = 0;
    virtual void present(gpu::CommandBuffer* cmd) = 0;
    virtual void endFrame(SurfaceFrame* frame) = 0;

    // ... existing cleanup ...
    virtual void cleanup() = 0;

protected:
    // ... existing members ...
    Size2D          _size;
    PixelFormat     _colorFormat = PixelFormat::Undefined;
    PixelFormat     _depthFormat = PixelFormat::Undefined;
    Device*         _device         = nullptr;
    PhysicalDevice* _physicalDevice = nullptr;

    // NEW: frame counter, incremented in endFrame()
    uint64_t _frameCounter = 0;

    // ... existing friends ...
};
```

## Implementation Details

- `inFlightFrames()` is a **pure virtual** method — every concrete surface must implement it
- `frameCounter()` is a **non-virtual inline** getter in the base class
- `_frameCounter` is **protected** so concrete surfaces can increment it in their `endFrame()` implementations
- The default value is `0`; it is never reset (monotonically increasing `uint64_t`)

## Integration Points

- `CleanupManager::defer()` will read `frameCounter()` and `inFlightFrames()` to compute `targetFrame`
- `CleanupManager::flushDeferred()` will compare `targetFrame` against `frameCounter()`
- `FrameResourceRing::create()` will use `inFlightFrames()` instead of `imageCount()`

## Notes

- `imageCount()` is **NOT removed** — it serves a different purpose (swapchain image count)
- `inFlightFrames()` returns a fixed value per surface type: 2 for window, 1 for offscreen
- The frame counter overflows after 2^64 frames — not a practical concern (584 billion years at 60fps)
