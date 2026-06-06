# Step 5 — `Surface` frame lifecycle + present-coupled submit

**Goal:** wire `beginFrame()` / `present(cmd)` / `endFrame(frame)` / `cleanup()` into `Surface`,
add the Vulkan per-frame synchronization, and upgrade `vk::Queue::submit()` to wait/signal the
frame sync objects and run `queuePresent`. After this step the full loop from the design
sketch works end-to-end.

Adding the pure virtuals to `gpu::Surface` forces **all four** concrete surfaces (vk/metal ×
window/offscreen) to implement them in this same step → build never breaks.

Depends on: step 02 (`SurfaceFrame`), step 03 (`CommandBuffer`), step 04 (`Queue`).

## Files

- **Modify** `lib/include/bg2e/gpu/Surface.hpp` (add pure virtuals + base `cleanup`)
- **Modify** `lib/include/bg2e/gpu/vk/WindowSurface.hpp` / `.cpp`
- **Modify** `lib/include/bg2e/gpu/vk/OffscreenSurface.hpp` / `.cpp`
- **Modify** `lib/include/bg2e/gpu/metal/WindowSurface.hpp` / `.cpp`
- **Modify** `lib/include/bg2e/gpu/metal/OffscreenSurface.hpp` / `.cpp`
- **Modify** `lib/src/bg2e/gpu/vk/Queue.cpp` (present-aware submit)

## `gpu::Surface` additions (abstract)

```cpp
#include <memory>

namespace bg2e {
namespace gpu {

class SurfaceFrame;
class CommandBuffer;

class BG2E_API Surface {
public:
    virtual ~Surface() = default;

    // ... existing members unchanged ...

    // Frame lifecycle
    virtual std::shared_ptr<SurfaceFrame> beginFrame() = 0;
    virtual void present(gpu::CommandBuffer* cmd) = 0;
    virtual void endFrame(SurfaceFrame* frame) = 0;

    virtual void cleanup() = 0;   // promote to base (WindowSurface already pure; Offscreen overrides)
};

}
}
```

> `cleanup()` is already pure in `WindowSurface` and concrete in `OffscreenSurface`; promoting the
> declaration to the base is compatible with both. Keep `WindowSurface`/`OffscreenSurface`
> declarations as they are.

## Vulkan window surface

> **Extension functions:** all Vulkan calls that come from extensions (`VK_KHR_swapchain`,
> `VK_KHR_synchronization2`, etc.) must use the function pointers loaded at runtime in
> `bg2e::gpu::vk::extensions.hpp` — never the direct `vk*` symbols. The affected calls in this
> step are: `acquireNextImage`, `queueSubmit2` and `queuePresent`. Core Vulkan 1.0 calls
> (`vkCreateSemaphore`, `vkCreateFence`, `vkWaitForFences`, `vkResetFences`, `vkDestroySemaphore`,
> `vkDestroyFence`) remain as direct calls.

### Per-frame sync objects

**The number of frames in flight is *not* a fixed constant.** It is derived from the actual number
of swapchain images, which is platform/driver dependent (e.g. an Apple M-series GPU creates 3
images). Hard-coding e.g. `2` while the swapchain has `3` images makes the CPU recycle per-frame
sync objects while the GPU is still using them — the validation layers flag this as a
synchronization hazard. So the storage is `std::vector`, sized at `createRenderTarget` time.

There are two distinct index spaces:

- **Per frame-in-flight** (indexed by `_currentFrame`, the CPU pacing counter):
  `imageAvailable` semaphores, `inFlight` fences and the `vk::SurfaceFrame` pool. Count =
  `_framesInFlight = swapchain image count`.
- **Per swapchain image** (indexed by the acquired `imageIndex`): the `renderFinished` semaphores
  (because `vkQueuePresentKHR` waits on them — a semaphore cannot be reused until its present has
  completed) and an `imagesInFlight` fence-alias table (to avoid reusing an image still being
  rendered by another in-flight frame).

`vk/WindowSurface.hpp` new members:

```cpp
uint32_t _framesInFlight = 0;
uint32_t _currentFrame   = 0;

// per frame-in-flight
std::vector<VkSemaphore> _imageAvailable;
std::vector<VkFence>     _inFlight;
std::vector<std::shared_ptr<vk::SurfaceFrame>> _frames;

// per swapchain image
std::vector<VkSemaphore> _renderFinished;
std::vector<VkFence>     _imagesInFlight; // non-owning aliases of _inFlight
```

Creation (in `createRenderTarget`, after `_colorImages` is filled, using `Info::semaphoreCreateInfo()`
/ `Info::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT)` — both exist):

```cpp
_framesInFlight = static_cast<uint32_t>(_colorImages.size());
_currentFrame   = 0;

_imageAvailable.resize(_framesInFlight, VK_NULL_HANDLE);
_inFlight.resize(_framesInFlight, VK_NULL_HANDLE);
_frames.resize(_framesInFlight);
for (uint32_t i = 0; i < _framesInFlight; ++i) {
    vkCreateSemaphore(dev, &semInfo, nullptr, &_imageAvailable[i]);
    vkCreateFence(dev, &fenceInfo /*signaled*/, nullptr, &_inFlight[i]);
    _frames[i] = std::make_shared<vk::SurfaceFrame>();
}

_renderFinished.resize(_colorImages.size(), VK_NULL_HANDLE);
for (auto& sem : _renderFinished)
    vkCreateSemaphore(dev, &semInfo, nullptr, &sem);

_imagesInFlight.assign(_colorImages.size(), VK_NULL_HANDLE);
```

Destruction (in `releaseRenderTarget`, before destroying the swapchain): destroy every
`_inFlight` fence and every `_imageAvailable` / `_renderFinished` semaphore (range-for over the
vectors), then `clear()` all vectors (including `_imagesInFlight`, which owns nothing) and reset
`_framesInFlight = _currentFrame = 0`. Guard with `_vkDevice != VK_NULL_HANDLE`.

### `beginFrame()`

```cpp
std::shared_ptr<SurfaceFrame> WindowSurface::beginFrame()
{
    vkWaitForFences(_vkDevice, 1, &_inFlight[_currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    VkResult r = acquireNextImage(_vkDevice, _swapchain, UINT64_MAX,
        _imageAvailable[_currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (r == VK_ERROR_OUT_OF_DATE_KHR) { resize(_size); return beginFrame(); }

    // Do not reuse an image that a previous frame is still rendering into.
    if (_imagesInFlight[imageIndex] != VK_NULL_HANDLE)
        vkWaitForFences(_vkDevice, 1, &_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    _imagesInFlight[imageIndex] = _inFlight[_currentFrame];

    vkResetFences(_vkDevice, 1, &_inFlight[_currentFrame]);

    auto frame = _frames[_currentFrame];
    frame->setColorImage(_colorImages[imageIndex].get());
    frame->setDepthImage(_depthImage.get());
    frame->setImageIndex(imageIndex);
    frame->setSwapchain(_swapchain);
    frame->setImageAvailable(_imageAvailable[_currentFrame]);
    frame->setRenderFinished(_renderFinished[imageIndex]);   // per image, not per frame
    frame->setInFlightFence(_inFlight[_currentFrame]);
    return frame;
}
```

### `present(cmd)`

Vulkan defers the actual `queuePresent` to `submit()`. `present` just attaches the current
frame to the command buffer so `submit` can find the sync objects and swapchain:

```cpp
void WindowSurface::present(gpu::CommandBuffer* cmd)
{
    auto* vkCmd = dynamic_cast<vk::CommandBuffer*>(cmd);
    vkCmd->setPresentFrame(_frames[_currentFrame].get());
}
```

### `endFrame(frame)`

```cpp
void WindowSurface::endFrame(SurfaceFrame*)
{
    _currentFrame = (_currentFrame + 1) % _framesInFlight;
}
```

## Vulkan: present-coupled `submit()` (`vk/Queue.cpp`)

Replace the plain submit from step 04 with a present-aware version:

```cpp
void Queue::submit(gpu::CommandBuffer* cmd) const
{
    auto* vkCmd = dynamic_cast<vk::CommandBuffer*>(cmd);
    auto* frame = vkCmd->presentFrame();   // nullptr for non-present (e.g. immediateSubmit)

    VkCommandBufferSubmitInfo cmdInfo = Info::commandBufferSubmitInfo(vkCmd->handle());

    if (frame) {
        VkSemaphoreSubmitInfo wait   = Info::semaphoreSubmitInfo(
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, frame->imageAvailable());
        VkSemaphoreSubmitInfo signal = Info::semaphoreSubmitInfo(
            VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, frame->renderFinished());
        VkSubmitInfo2 submit = Info::submitInfo(&cmdInfo, &signal, &wait);
        queueSubmit2(_queue, 1, &submit, frame->inFlightFence());

        VkSemaphore    waitSem = frame->renderFinished();
        VkSwapchainKHR sc      = frame->swapchain();
        uint32_t       idx     = frame->imageIndex();
        VkPresentInfoKHR present = Info::presentInfo(sc, waitSem, idx);
        queuePresent(_queue, &present);   // present queue == graphics queue here
    } else {
        VkSubmitInfo2 submit = Info::submitInfo(&cmdInfo, nullptr, nullptr);
        queueSubmit2(_queue, 1, &submit, VK_NULL_HANDLE);
    }
}
```

> `Info::semaphoreSubmitInfo`, `Info::commandBufferSubmitInfo`, `Info::submitInfo(...)` and
> `Info::presentInfo(...)` already exist in `vk/Info.hpp`. Presentation uses the graphics queue
> handle; on this engine graphics and present families are typically identical. If a dedicated
> present family is later required, pass the present `VkQueue` through the frame.

## Vulkan offscreen surface

`vk::OffscreenSurface` must satisfy the new interface but has no swapchain:

- **`beginFrame()`** — return a `vk::SurfaceFrame` (kept as a single member) pointing at the
  offscreen color image (index 0) and depth image; leave swapchain/semaphores/fence null.
- **`present(cmd)`** — no-op (do not call `setPresentFrame`, so `submit` does a plain submit).
- **`endFrame(frame)`** — no-op.
- **`cleanup()`** — already exists via `OffscreenSurface::cleanup()` (calls `releaseRenderTarget`);
  keep it.

## Metal window surface

### `beginFrame()`

```cpp
std::shared_ptr<SurfaceFrame> WindowSurface::beginFrame()
{
    auto* drawable = _layer->nextDrawable();           // CA::MetalDrawable*
    auto frame = std::make_shared<metal::SurfaceFrame>();
    frame->setDrawable(drawable);

    auto colorImg = std::make_unique<metal::Image>();
    colorImg->initFromDrawableTexture(metalDevice, drawable->texture(),
                                      _colorFormat, _size);   // non-owning wrap (step 02)
    frame->setColorImage(std::move(colorImg));
    frame->setDepthImage(_depthImage.get());
    return frame;
}
```

(Metal frames are created fresh each iteration; the drawable is autoreleased and presented via the
command buffer, so no manual retain of the drawable is required as long as `present`/`commit`
happen within the same loop turn.)

### `present(cmd)`

Metal schedules drawable presentation on the `MTL::CommandBuffer` **before** `commit()` (which
happens in `Queue::submit`, called after `present` in the loop). `present` needs the frame's
drawable; store the "current frame" when `beginFrame` runs, or have `present` look it up. Simplest:
keep a `std::weak_ptr`/raw pointer `_currentFrame` set in `beginFrame` and cleared in `endFrame`.

```cpp
void WindowSurface::present(gpu::CommandBuffer* cmd)
{
    auto* mtlCmd = dynamic_cast<metal::CommandBuffer*>(cmd);
    mtlCmd->handle()->presentDrawable(_currentFrame->drawable());
}
```

> Add a `metal::SurfaceFrame* _currentFrame = nullptr;` member to `metal::WindowSurface`, set it in
> `beginFrame` (`_currentFrame = frame.get();`) and clear it in `endFrame`.

### `endFrame(frame)`

```cpp
void WindowSurface::endFrame(SurfaceFrame*)
{
    _currentFrame = nullptr;   // release transient per-frame reference
}
```

Metal manages drawable availability through `CA::MetalLayer`; no in-flight index bookkeeping is
required for this minimal loop (the drawable pool already throttles to `maximumDrawableCount`).

## Metal offscreen surface

- **`beginFrame()`** — return a `metal::SurfaceFrame` whose color image is the offscreen color
  texture and depth is the offscreen depth image. No drawable.
- **`present(cmd)`** — no-op.
- **`endFrame(frame)`** — no-op.
- **`cleanup()`** — already provided via base `OffscreenSurface::cleanup()`.

All Metal bodies live in the `BG2E_IS_MAC` branch; the non-mac branch throws the standard
"Metal backend is not available on this platform" error (and `beginFrame` returns `nullptr`).

## Shutdown ordering

The sketch's shutdown is `device->waitIdle(); surface->cleanup(); device->cleanup();
instance->cleanup();`. Keep this order: `surface->cleanup()` releases sync objects + render target
(which depend on the device) before `device->cleanup()` destroys the device and the queues' command
pools (step 04). No change needed beyond what each `cleanup()` already does.

## Build check

All four surfaces implement the new pure virtuals here; `vk::Queue::submit` is upgraded in the same
step. `02_device` does not call the new methods, so it still compiles. The full loop is now
functional. Build stays green on all platforms.
