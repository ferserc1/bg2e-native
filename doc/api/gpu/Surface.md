# Surface

**Header:** `<bg2e/gpu/Surface.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API Surface {
public:
    virtual ~Surface() = default;

    virtual bool isOffscreen() const = 0;
    virtual bool isValid() const = 0;

    virtual uint32_t width() const = 0;
    virtual uint32_t height() const = 0;

    void setWidth(uint32_t w);
    void setHeight(uint32_t h);

protected:
    uint32_t _width = 0;
    uint32_t _height = 0;
};
```

Abstract base for rendering surfaces. Holds width and height state common to
both window and offscreen surfaces. Subclassed by `WindowSurface` and
`OffscreenSurface`.

This class has no backend-specific implementations. The `vk` and `metal`
namespaces only provide concrete subclasses of `WindowSurface` and
`OffscreenSurface`.

---

## Methods

### `virtual bool isOffscreen() const = 0`

Returns `true` if this is an offscreen surface, `false` for window surfaces.

### `virtual bool isValid() const = 0`

Returns `true` if the surface is properly initialized and ready for rendering.

### `virtual uint32_t width() const = 0`

Returns the surface width in pixels.

### `virtual uint32_t height() const = 0`

Returns the surface height in pixels.

### `void setWidth(uint32_t w)`

Sets the surface width. Non-virtual inline setter.

| Parameter | Type       | Description          |
|-----------|------------|----------------------|
| `w`       | `uint32_t` | New width in pixels. |

### `void setHeight(uint32_t h)`

Sets the surface height. Non-virtual inline setter.

| Parameter | Type       | Description          |
|-----------|------------|----------------------|
| `h`       | `uint32_t` | New height in pixels.|
