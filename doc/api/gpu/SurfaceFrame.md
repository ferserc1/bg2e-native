# SurfaceFrame

**Header:** `<bg2e/gpu/SurfaceFrame.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
class BG2E_API SurfaceFrame {
public:
    virtual ~SurfaceFrame() = default;

    virtual gpu::Image* colorImage() const = 0;
    virtual gpu::Image* depthImage() const = 0;

    virtual bool isValid() const = 0;
};
```

Represents a single frame from a surface's swapchain. Obtained via
`Surface::beginFrame()` and passed to `Surface::endFrame()` when rendering is
complete.

---

## Methods

### `virtual gpu::Image* colorImage() const = 0`

Returns the color image for this frame. Use it for layout transitions and as
the rendering target.

### `virtual gpu::Image* depthImage() const = 0`

Returns the depth image for this frame.

### `virtual bool isValid() const = 0`

Returns `true` if the frame was successfully acquired.

---

## Example

```cpp
auto frame = surface->beginFrame();
if (!frame || !frame->isValid()) return;

cmd->begin();
cmd->transition(frame->colorImage(), gpu::ImageLayout::ColorAttachment);
cmd->transition(frame->depthImage(), gpu::ImageLayout::DepthAttachment);
cmd->beginRendering(frame.get());
// ... draw ...
cmd->endRendering();
cmd->transition(frame->colorImage(), gpu::ImageLayout::Present);

surface->present(cmd.get());
cmd->end();
device->graphicsQueue().submit(cmd.get());
surface->endFrame(frame.get());
```

---

## vk::SurfaceFrame

**Header:** `<bg2e/gpu/vk/SurfaceFrame.hpp>`
**Namespace:** `bg2e::gpu::vk`
**Inherits:** `gpu::SurfaceFrame`

```cpp
class SurfaceFrame : public gpu::SurfaceFrame {
public:
    gpu::Image* colorImage() const override;
    gpu::Image* depthImage() const override;
    bool isValid() const override;

    uint32_t imageIndex() const;
    VkSemaphore imageAvailable() const;
    VkSemaphore renderFinished() const;
    VkFence inFlightFence() const;
};
```

Vulkan frame object. Tracks swapchain image index and synchronization
primitives (semaphores and fences).

### Vulkan-specific methods

#### `uint32_t imageIndex() const`

Returns the swapchain image index for this frame.

#### `VkSemaphore imageAvailable() const`

Returns the semaphore signaled when the swapchain image is available.

#### `VkSemaphore renderFinished() const`

Returns the semaphore signaled when rendering is complete.

#### `VkFence inFlightFence() const`

Returns the fence signaled when the frame's GPU work is finished.

---

## metal::SurfaceFrame

**Header:** `<bg2e/gpu/metal/SurfaceFrame.hpp>`
**Namespace:** `bg2e::gpu::metal`
**Inherits:** `gpu::SurfaceFrame`

Metal frame object. Wraps the drawable and associated textures.

## Internal completion records

Backend friends associate submitted commands with this frame and append immutable
completion records. Surface::beginFrame waits those records before reusing an
occupied resource slot. They outlive wrapper reuse and cache terminal completion
before a Vulkan frame fence is reset. Window endFrame performs bookkeeping only;
it requires an acquired frame with a recorded submission.

The generic tracker supports multiple records per slot; the Vulkan swapchain
submit path currently restricts one presentation send per acquired frame.
Drawable/image references are borrowed for the frame's lifetime. See
[implementation](Submission_tracking_and_waitIdle.md#surface-slots-and-reusable-fence-correctness).
