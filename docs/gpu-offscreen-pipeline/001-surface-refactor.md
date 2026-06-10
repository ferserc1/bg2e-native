# Step 001 — Intermediate `vk::Surface` / `metal::Surface`

**Type:** Refactor
**Depends on:** none
**Enables:** cleaner base for all later steps (not a hard dependency)

## Goal

Remove the duplication between the window and offscreen surfaces of each backend by
introducing a backend-common intermediate class, `vk::Surface` and `metal::Surface`, that
holds the code shared by *both* surface kinds: the cached down-cast device pointer, the
depth-image member and its create/resize/release helpers, and the `SurfaceFrame` storage
helpers. The public typed factory (`createWindowSurface` → `gpu::WindowSurface`,
`createOffscreenSurface` → `gpu::OffscreenSurface`) must keep working unchanged.

Because the common class is shared across the Window/Offscreen split while both
`gpu::WindowSurface` and `gpu::OffscreenSurface` must remain in the concrete classes'
ancestry, this is a diamond, resolved with **virtual inheritance** of `gpu::Surface`.

## Resulting hierarchy

```
gpu::Surface                                  (virtual base — unchanged interface)
├── gpu::WindowSurface    : public virtual gpu::Surface
├── gpu::OffscreenSurface : public virtual gpu::Surface
└── vk::Surface           : public virtual gpu::Surface     (common vk code)
        ▲
        │   (and metal::Surface : public virtual gpu::Surface)
vk::WindowSurface     : public gpu::WindowSurface,    public vk::Surface
vk::OffscreenSurface  : public gpu::OffscreenSurface, public vk::Surface
metal::WindowSurface  : public gpu::WindowSurface,    public metal::Surface
metal::OffscreenSurface: public gpu::OffscreenSurface, public metal::Surface
```

## Changes

### 1. Make the gpu-level markers virtual bases

- `lib/include/bg2e/gpu/WindowSurface.hpp`:
  `class BG2E_API WindowSurface : public virtual Surface` (add `virtual`).
- `lib/include/bg2e/gpu/OffscreenSurface.hpp`:
  `class BG2E_API OffscreenSurface : public virtual Surface` (add `virtual`).
  Keep the `explicit OffscreenSurface(const Size2D&)` ctor (it assigns the virtual-base
  member `_size` in its body, which is valid).

> The `friend class vk::Surface; friend class metal::Surface;` declarations should be added
> to `gpu::Surface` (alongside the existing Backend/Device friends) so the intermediate can
> call the protected `createRenderTarget` lifecycle hooks if needed. The existing friends stay.

### 2. New `gpu/vk/Surface.hpp` + `gpu/vk/Surface.cpp`

```cpp
namespace bg2e::gpu::vk {

class Device;

class Surface : public virtual gpu::Surface {
protected:
    // Down-cast helper, cached on createRenderTarget.
    vk::Device* vkDevice() const { return _vkDevice; }

    // Common depth render-target management (used by both Window and Offscreen).
    void createDepthTarget(const Size2D& size, PixelFormat format);
    void resizeDepthTarget(const Size2D& size);
    void releaseDepthTarget();

    vk::Device*                 _vkDevice = nullptr;
    std::unique_ptr<vk::Image>  _depthImage;
};

}
```

- `createDepthTarget`: if `format != PixelFormat::Undefined`, build `_depthImage` via
  `buildDepthImage(_vkDevice, size, format)`.
- `resizeDepthTarget` / `releaseDepthTarget`: guard on `_depthImage`.
- `_vkDevice` is set inside each concrete `createRenderTarget` (down-cast once).

### 3. Re-parent `vk::WindowSurface` / `vk::OffscreenSurface`

- `vk::WindowSurface : public gpu::WindowSurface, public vk::Surface`.
- `vk::OffscreenSurface : public gpu::OffscreenSurface, public vk::Surface`.
- Move the `_depthImage` member out of each concrete class into `vk::Surface`; replace the
  inline depth create/resize/release blocks in their `.cpp` with calls to the new helpers.
- `vk::OffscreenSurface`'s ctor still forwards `size` to `gpu::OffscreenSurface(size)`; under
  virtual inheritance the most-derived class also default-initializes the `gpu::Surface`
  virtual base (implicitly). No behavioural change.
- `isOffscreen()` continues to come from the `gpu::WindowSurface` / `gpu::OffscreenSurface`
  marker — no change needed in the concrete classes.

### 4. Mirror for Metal

- New `gpu/metal/Surface.hpp` + `gpu/metal/Surface.cpp` with `metal::Device* _metalDevice`
  and `std::unique_ptr<metal::Image> _depthImage` + the same three helpers, all under
  `BG2E_IS_MAC` (guarded-stub helpers otherwise so the file compiles on Linux/Windows).
- Re-parent `metal::WindowSurface` / `metal::OffscreenSurface` and replace their inline depth
  blocks with the helper calls.

## Notes / pitfalls

- **Virtual inheritance + friends:** the existing `friend class vk::Device;` etc. on
  `gpu::Surface` still grant access; `protected` members of `gpu::Surface` (`_size`,
  `_colorFormat`, `_depthFormat`, `_device`, `_physicalDevice`) remain reachable from the
  intermediate because it derives `gpu::Surface`.
- Keep the down-cast pattern identical to today (`dynamic_cast<vk::Device*>(device)`); just
  store the result in `_vkDevice` instead of a local.
- Do **not** move the color-image members: window stores swapchain images (a vector) while
  offscreen stores a single owned image — these stay in the concrete classes. Only the
  **depth** image and the cached device pointer are genuinely common.

## Validation

- `bg2e` and examples `03_offscreen_device`, `04_clear_loop`, `05_simple_triangle` build and
  behave **exactly** as before (window present + offscreen device creation unchanged).
- No public signature changes: the factory still returns `gpu::WindowSurface` /
  `gpu::OffscreenSurface`.
