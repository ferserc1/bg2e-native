# Step 02: vk::WindowSurface Implementation

## Files to Modify

- `lib/include/bg2e/gpu/vk/WindowSurface.hpp`
- `lib/src/bg2e/gpu/vk/WindowSurface.cpp`

## Interface Changes

### Header (`WindowSurface.hpp`)

Add `inFlightFrames()` override to the public interface:

```cpp
class WindowSurface : public gpu::WindowSurface, public Surface {
public:
    // ... existing overrides ...

    uint32_t    imageCount() const override;       // stays
    uint32_t    inFlightFrames() const override;   // NEW
    uint32_t    currentFrameIndex() const override;
    // ...
};
```

### Source (`WindowSurface.cpp`)

**1. Implement `inFlightFrames()`:**

```cpp
uint32_t WindowSurface::inFlightFrames() const
{
    return 2;
}
```

**2. Increment `_frameCounter` in `endFrame()`:**

Current code:
```cpp
void WindowSurface::endFrame(gpu::SurfaceFrame*)
{
    _currentFrame = (_currentFrame + 1) % _framesInFlight;
}
```

New code:
```cpp
void WindowSurface::endFrame(gpu::SurfaceFrame*)
{
    _currentFrame = (_currentFrame + 1) % _framesInFlight;
    ++_frameCounter;
}
```

## Implementation Details

- `inFlightFrames()` returns a hardcoded `2` — this matches the engine's design for window surfaces
- The `_frameCounter` increment happens at the **end** of `endFrame()`, after the frame index is updated
- `_frameCounter` is inherited from `gpu::Surface` (protected member)
- `imageCount()` remains unchanged — it returns the actual swapchain image count (platform-dependent, e.g. 3 on Apple M-series)

## Integration Points

- `beginFrame()` waits on the fence for `_inFlight[_currentFrame]` — this is the point where previous frame resources are guaranteed to be no longer in use
- `flushDeferred()` should be called **after** `endFrame()` in the render loop, so the counter has been incremented
- The deferred cleanup closures will execute when `frameCounter() >= targetFrame`, where `targetFrame = frameAtScheduleTime + 2`

## Notes

- The Vulkan `WindowSurface` uses `_framesInFlight` (derived from swapchain image count) for its own internal frame cycling, but `inFlightFrames()` for the deferred cleanup API always returns 2 as a fixed value
- These are separate concepts: `_framesInFlight` controls Vulkan sync object indexing, while `inFlightFrames()` controls cleanup delay
