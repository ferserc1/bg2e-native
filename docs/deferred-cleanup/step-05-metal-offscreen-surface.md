# Step 05: metal::OffscreenSurface Implementation

## Files to Modify

- `lib/include/bg2e/gpu/metal/OffscreenSurface.hpp`
- `lib/src/bg2e/gpu/metal/OffscreenSurface.cpp`

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

**Implement `inFlightFrames()` (macOS block):**

```cpp
uint32_t OffscreenSurface::inFlightFrames() const { return 1; }
```

Also add the non-macOS stub:

```cpp
uint32_t OffscreenSurface::inFlightFrames() const { return 0; }
```

## Implementation Details

- `inFlightFrames()` returns `1` on macOS, `0` on other platforms
- **No frame counter increment** — offscreen rendering is single-shot
- `endFrame()` remains a no-op (unchanged)
- `imageCount()` remains `1` (unchanged)

## Integration Points

- Same as `vk::OffscreenSurface` — deferred cleanup is primarily useful for windowed rendering
- For offscreen batch work, call `flushAllDeferred()` after `device->waitIdle()`

## Notes

- The `#else` block (non-macOS) must also declare `inFlightFrames()` to avoid compilation errors
