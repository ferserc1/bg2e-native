# Step 3 — Migrate existing `gpu` size APIs to `Size2D`

**Goal:** Make `Size2D` the canonical size type in the `gpu` framework before the new
render-target/image code is written, so it uses `Size2D` from the start. This is a
behaviour-preserving refactor: same runtime behaviour, signatures updated.

## Files

- **Modify** `lib/include/bg2e/gpu/Surface.hpp`
- **Modify** `lib/include/bg2e/gpu/OffscreenSurface.hpp`
- **Modify** `lib/include/bg2e/gpu/Backend.hpp`
- **Modify** `lib/include/bg2e/gpu/vk/WindowSurface.hpp` + `lib/src/bg2e/gpu/vk/WindowSurface.cpp`
- **Modify** `lib/include/bg2e/gpu/metal/WindowSurface.hpp` + `lib/src/bg2e/gpu/metal/WindowSurface.cpp`
- **Modify** `lib/include/bg2e/gpu/vk/Backend.hpp` + `lib/src/bg2e/gpu/vk/Backend.cpp`
- **Modify** `lib/include/bg2e/gpu/metal/Backend.hpp` + `lib/src/bg2e/gpu/metal/Backend.cpp`
- **Modify** `examples/gpu/03_offscreen_device/src/main.cpp`

## `Surface.hpp`

Replace the two scalar members and their setters with a single `Size2D`, keep
`width()/height()` as inline accessors. The `width()`/`height()` virtuals stay (subclasses can
still override, e.g. the vk window surface queries SDL live), but the base offers a stored
`Size2D`.

```cpp
#include <bg2e/gpu/Common.hpp>   // Size2D

class BG2E_API Surface {
public:
    virtual ~Surface() = default;

    virtual bool isOffscreen() const = 0;
    virtual bool isValid() const = 0;

    virtual const Size2D& size() const { return _size; }
    void setSize(const Size2D& s) { _size = s; }

    // Thin accessors kept for convenience / source compatibility.
    virtual uint32_t width()  const { return _size.width;  }
    virtual uint32_t height() const { return _size.height; }

protected:
    Size2D _size;
};
```

> Removed: `uint32_t _width`, `uint32_t _height`, `setWidth`, `setHeight`. Any internal writes
> to `_width`/`_height` become writes to `_size`.

## `OffscreenSurface.hpp`

```cpp
class BG2E_API OffscreenSurface : public Surface {
public:
    explicit OffscreenSurface(const Size2D& size) { _size = size; }

    bool isOffscreen() const override { return true; }
    bool isValid()     const override { return true; }   // refined in Step 7
};
```

## `Backend.hpp` (abstract) + concrete backends

Change the factory signature:

```cpp
[[nodiscard]] virtual std::unique_ptr<gpu::OffscreenSurface>
    createOffscreenSurface(const Size2D& size) const = 0;
```

Update `vk::Backend` / `metal::Backend` headers + `.cpp` overrides accordingly, forwarding the
`Size2D` to the concrete `OffscreenSurface(size)` constructor (vk/metal `OffscreenSurface`
ctors take `const Size2D&` and pass to `gpu::OffscreenSurface`).

## `vk::WindowSurface` / `metal::WindowSurface`

- Keep the live `width()/height()` overrides where they exist (vk queries SDL).
- In `create(Instance*)`, replace `_width = ...; _height = ...;` with
  `_size = Size2D{ uint32_t(w), uint32_t(h) };`.
- `metal::WindowSurface::cleanup()` resets `_size = Size2D{};` instead of `_width/_height = 0`.

## `vk::OffscreenSurface` / `metal::OffscreenSurface`

Constructor changes from `(uint32_t, uint32_t)` to `(const Size2D&)`:

```cpp
explicit OffscreenSurface(const Size2D& size) : gpu::OffscreenSurface(size) {}
```

## Example update

`examples/gpu/03_offscreen_device/src/main.cpp` line ~58:

```cpp
auto surface = backend->createOffscreenSurface(gpu::Size2D{ 800, 600 });
```

`examples/gpu/02_device` does not call `createOffscreenSurface`, but verify any `width()/height()`
usage still compiles (it does — accessors retained).

## Compile check

All call sites updated in the same step. Behaviour is identical (the surface still reports the
same size). Build + both examples stay green.
