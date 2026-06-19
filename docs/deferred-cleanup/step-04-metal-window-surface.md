# Step 04: metal::WindowSurface Implementation

## Files to Modify

- `lib/include/bg2e/gpu/metal/WindowSurface.hpp`
- `lib/src/bg2e/gpu/metal/WindowSurface.cpp`

## Interface Changes

### Header (`WindowSurface.hpp`)

Add `inFlightFrames()` override:

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

**1. Implement `inFlightFrames()` (macOS block):**

```cpp
uint32_t WindowSurface::inFlightFrames() const { return 2; }
```

Also add the non-macOS stub (inside the `#else` block):

```cpp
uint32_t WindowSurface::inFlightFrames() const { return 0; }
```

**2. Increment `_frameCounter` in `endFrame()` (macOS block):**

Current code:
```cpp
void WindowSurface::endFrame(gpu::SurfaceFrame*)
{
    _currentFrame = nullptr;
}
```

New code:
```cpp
void WindowSurface::endFrame(gpu::SurfaceFrame*)
{
    _currentFrame = nullptr;
    ++_frameCounter;
}
```

## Implementation Details

- `inFlightFrames()` returns `2` on macOS, `0` on other platforms (Metal unavailable)
- The `_frameCounter` increment happens at the **end** of `endFrame()`, after clearing `_currentFrame`
- Metal's `WindowSurface` does not have a `_framesInFlight` counter like Vulkan — it uses `nextDrawable()` for per-frame resources
- `imageCount()` returns `_imageCount` (set to 3 in `createRenderTarget()`) and is unchanged

## Integration Points

- Metal does not have explicit fences like Vulkan; synchronization is handled by the Metal runtime
- `flushDeferred()` should still be called after `endFrame()` for consistency with the Vulkan path
- The frame counter increment is safe because Metal's `endFrame()` is called from the main thread

## Notes

- The Metal `WindowSurface` sets `_imageCount = 3` (maximum drawable count), but `inFlightFrames()` returns 2 — these are intentionally different values
- The `#else` block (non-macOS) must also declare `inFlightFrames()` to avoid compilation errors
