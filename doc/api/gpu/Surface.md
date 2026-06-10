# Surface

**Header:** `<bg2e/gpu/Surface.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API Surface {
public:
    virtual ~Surface() = default;

    virtual bool isOffscreen() const = 0;
    virtual bool isValid() const = 0;

    const Size2D& size() const;
    void setSize(const Size2D& s);

    virtual uint32_t width() const;
    virtual uint32_t height() const;

    PixelFormat colorFormat() const;
    PixelFormat depthFormat() const;
    void setColorFormat(PixelFormat f);
    void setDepthFormat(PixelFormat f);

    virtual void resize(const Size2D& size) = 0;
    virtual void releaseRenderTarget() = 0;

    virtual uint32_t    imageCount() const = 0;
    virtual gpu::Image* colorImage(uint32_t index) const = 0;
    virtual gpu::Image* depthImage() const = 0;

    // Frame lifecycle
    virtual std::shared_ptr<SurfaceFrame> beginFrame() = 0;
    virtual void present(gpu::CommandBuffer* cmd) = 0;
    virtual void endFrame(SurfaceFrame* frame) = 0;

    virtual void cleanup() = 0;
};
```

Abstract base for rendering surfaces. Holds width, height, and pixel format
state common to both window and offscreen surfaces. Subclassed by
`WindowSurface` and `OffscreenSurface`.

This class has no backend-specific implementations. The `vk` and `metal`
namespaces only provide concrete subclasses of `WindowSurface` and
`OffscreenSurface`.

---

## Methods

### `virtual bool isOffscreen() const = 0`

Returns `true` if this is an offscreen surface, `false` for window surfaces.

### `virtual bool isValid() const = 0`

Returns `true` if the surface is properly initialized and ready for rendering.

### `const Size2D& size() const`

Returns the surface dimensions as a `Size2D` struct.

### `void setSize(const Size2D& s)`

Sets the surface dimensions.

### `virtual uint32_t width() const`

Returns the surface width in pixels.

### `virtual uint32_t height() const`

Returns the surface height in pixels.

### `PixelFormat colorFormat() const`

Returns the pixel format used for the color attachment.

### `PixelFormat depthFormat() const`

Returns the pixel format used for the depth attachment.

### `void setColorFormat(PixelFormat f)`

Sets the color attachment pixel format.

### `void setDepthFormat(PixelFormat f)`

Sets the depth attachment pixel format.

### `virtual void resize(const Size2D& size) = 0`

Resizes the surface and recreates internal render targets. Called when the
window is resized (for window surfaces) or when the offscreen dimensions
change.

| Parameter | Type     | Description          |
|-----------|----------|----------------------|
| `size`    | `Size2D` | New surface size.    |

### `virtual void releaseRenderTarget() = 0`

Releases the current render target resources without destroying the surface
itself. Useful before resizing or when temporarily detaching from a device.

### `virtual uint32_t imageCount() const = 0`

Returns the number of images in the swapchain (window surfaces) or 1 for
offscreen surfaces.

### `virtual gpu::Image* colorImage(uint32_t index) const = 0`

Returns the color image at the given swapchain index.

| Parameter | Type       | Description               |
|-----------|------------|---------------------------|
| `index`   | `uint32_t` | Swapchain image index.    |

### `virtual gpu::Image* depthImage() const = 0`

Returns the depth image for the surface.

### `virtual std::shared_ptr<SurfaceFrame> beginFrame() = 0`

Acquires the next frame from the swapchain. Returns a `SurfaceFrame` containing
the color and depth images for this frame. Returns `nullptr` if the frame
cannot be acquired (e.g., surface is out of date).

### `virtual void present(gpu::CommandBuffer* cmd) = 0`

Records presentation commands into the given command buffer. Must be called
after rendering is complete but before `endFrame()`.

| Parameter | Type               | Description                          |
|-----------|--------------------|--------------------------------------|
| `cmd`     | `gpu::CommandBuffer*` | The command buffer being recorded.|

### `virtual void endFrame(SurfaceFrame* frame) = 0`

Submits the frame for presentation and releases frame resources.

| Parameter | Type              | Description                    |
|-----------|-------------------|--------------------------------|
| `frame`   | `SurfaceFrame*`   | The frame returned by `beginFrame()`. |

### `virtual void cleanup() = 0`

Destroys the surface and releases all associated resources.

---

## Frame lifecycle

The typical render loop uses three surface methods in sequence:

```cpp
auto frame = surface->beginFrame();   // acquire next image
// ... record and submit commands ...
surface->present(cmd.get());          // record present
surface->endFrame(frame.get());       // submit and present
```
