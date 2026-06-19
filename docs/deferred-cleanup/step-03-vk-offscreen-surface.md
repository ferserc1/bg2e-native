# Step 03: vk::OffscreenSurface Implementation

## Files to Modify

- `lib/include/bg2e/gpu/vk/OffscreenSurface.hpp`
- `lib/src/bg2e/gpu/vk/OffscreenSurface.cpp`

## Interface Changes

### Header (`OffscreenSurface.hpp`)

Add `inFlightFrames()` override:

```cpp
class OffscreenSurface : public gpu::OffscreenSurface, public Surface {
public:
    // ... existing overrides ...

    uint32_t    imageCount() const override;       // stays
    uint32_t    inFlightFrames() const override;   // NEW
    uint32_t    currentFrameIndex() const override;
    // ...
};
```

### Source (`OffscreenSurface.cpp`)

**Implement `inFlightFrames()`:**

```cpp
uint32_t OffscreenSurface::inFlightFrames() const
{
    return 1;
}
```

## Implementation Details

- `inFlightFrames()` returns `1` — offscreen surfaces have no pipelining
- **No frame counter increment** — offscreen rendering is single-shot (no continuous loop)
- `endFrame()` remains a no-op for offscreen surfaces
- `imageCount()` remains `1` (unchanged)

## Integration Points

- Offscreen surfaces don't run a continuous render loop, so deferred cleanup is less useful
- However, `CleanupManager` still works correctly: `defer()` would set `targetFrame = frameCounter + 1`, and since the counter never increments, closures would only run if `flushAllDeferred()` is called explicitly
- For offscreen batch rendering, users should call `flushAllDeferred()` after `device->waitIdle()`

## Notes

- The offscreen `endFrame()` intentionally does NOT increment `_frameCounter` because there is no frame-to-frame pipelining
- `imageCount()` and `inFlightFrames()` both return 1 for offscreen, but they represent different concepts
