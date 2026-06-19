# Step 06: FrameResourceRing Update

## Files to Modify

- `lib/include/bg2e/gpu/FrameResourceRing.hpp`

## Interface Changes

In `FrameResourceRing<T>::create()`, change `surface->imageCount()` to `surface->inFlightFrames()`:

**Current code (line 50):**
```cpp
void create(
    gpu::Surface* surface,
    std::function<std::shared_ptr<T>(uint32_t)> factory)
{
    _surface = surface;
    uint32_t count = surface->imageCount();
    _resources.resize(count);
    for (uint32_t i = 0; i < count; ++i)
    {
        _resources[i] = factory(i);
    }
}
```

**New code:**
```cpp
void create(
    gpu::Surface* surface,
    std::function<std::shared_ptr<T>(uint32_t)> factory)
{
    _surface = surface;
    uint32_t count = surface->inFlightFrames();
    _resources.resize(count);
    for (uint32_t i = 0; i < count; ++i)
    {
        _resources[i] = factory(i);
    }
}
```

## Implementation Details

- Only one line changes: `imageCount()` → `inFlightFrames()`
- The ring size should match the number of concurrent frames, not the number of swapchain images
- For window surfaces: `inFlightFrames()=2`, so 2 copies of per-frame resources
- For offscreen: `inFlightFrames()=1`, so 1 copy

## Integration Points

- `FrameResourceRing::current()` uses `_surface->currentFrameIndex()` to index into the ring
- `currentFrameIndex()` returns `_currentFrame % _framesInFlight` (Vulkan) or `0` (offscreen/Metal)
- The ring size must match the number of possible `currentFrameIndex()` values

## Notes

- This change affects all existing examples that use `FrameResourceRing` (e.g., `07_uniform_buffers`)
- For Vulkan window surfaces, `imageCount()` could be 3 (Apple M-series) while `inFlightFrames()` is 2
- The existing `_framesInFlight` in `vk::WindowSurface` is set to `_colorImages.size()` (the actual swapchain image count), but `inFlightFrames()` returns 2 as a fixed value — these are separate concepts
- This means the FrameResourceRing will have 2 entries, while the Vulkan surface may cycle through 3 swapchain images. The ring index comes from `currentFrameIndex()` which is `_currentFrame % _framesInFlight`, so the ring and the surface stay synchronized
