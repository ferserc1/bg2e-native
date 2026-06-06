# Step 3 — `gpu::CommandBuffer` (abstract + backend implementations)

**Goal:** define the command-recording abstraction and implement it for both backends. Nothing
creates a `CommandBuffer` yet (that is `Queue::createCommandBuffer()` in step 04), so the build
stays green even though the implementations are complete.

Depends on: step 01 (`Color`, `ImageLayout`, `Image::currentLayout`) and step 02 (`SurfaceFrame`).

## Files

- **New** `lib/include/bg2e/gpu/CommandBuffer.hpp`
- **New** `lib/include/bg2e/gpu/vk/CommandBuffer.hpp`
- **New** `lib/src/bg2e/gpu/vk/CommandBuffer.cpp`
- **New** `lib/include/bg2e/gpu/metal/CommandBuffer.hpp`
- **New** `lib/src/bg2e/gpu/metal/CommandBuffer.cpp`
- **Modify** `lib/include/bg2e/gpu/all.hpp` (add abstract header include)

## `CommandBuffer.hpp` (abstract)

```cpp
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/gpu/Common.hpp>

namespace bg2e {
namespace gpu {

class Image;
class SurfaceFrame;

class BG2E_API CommandBuffer {
public:
    virtual ~CommandBuffer() = default;

    virtual void begin() = 0;
    virtual void end()   = 0;

    // Transition to a desired logical layout. Reads image->currentLayout(),
    // emits a backend transition only if different, then updates it.
    virtual void transition(gpu::Image* image, ImageLayout newLayout) = 0;

    // Dynamic-rendering scope bound to the given frame's color/depth images.
    virtual void beginRendering(gpu::SurfaceFrame* frame) = 0;
    virtual void endRendering() = 0;

    // Recorded as load-op clears applied by beginRendering/endRendering (see step 03 notes).
    virtual void clearColor(uint32_t attachmentIndex, const gpu::Color& color) = 0;
    virtual void clearDepth(float depth) = 0;

    virtual bool isValid() const = 0;
};

}
}
```

### Clear-value contract (both backends)

`clearColor` / `clearDepth` only **record** clear values; the actual clear happens through the
attachment load operation. To make the abstraction order-independent across backends, the contract
is:

- The loop calls `beginRendering(frame)` first, then `clearColor` / `clearDepth`, then
  `endRendering`.
- **Vulkan** may open the rendering scope lazily: `beginRendering` stores the frame and marks the
  scope active but does **not** call `vkCmdBeginRendering` yet; the first command that needs an
  active scope — here `endRendering` — flushes it, building `VkRenderingInfo` with the recorded
  clear values as `loadOp = CLEAR` and `storeOp = STORE`, calls `vkCmdBeginRendering`, then
  `vkCmdEndRendering`. (Equivalently, `beginRendering` can begin immediately with default clears
  and `clearColor/clearDepth` use `vkCmdClearAttachments`; the lazy form is preferred because it
  matches Metal and avoids a redundant clear.)
- **Metal** must defer encoder creation because clear values belong on the
  `MTL::RenderPassDescriptor` *before* `renderCommandEncoder` is created. `beginRendering`
  configures the descriptor (textures, default load/store); `clearColor/clearDepth` set
  `loadAction = Clear` + clear color/depth; `endRendering` creates the encoder from the finalized
  descriptor and immediately calls `endEncoding()`.

This keeps the same recorded sequence working on both backends.

## `ImageLayout` → backend mapping

Add a small mapping helper per backend (free function in the backend `common.hpp`/`.cpp` or a
`static` in the command buffer). Required entries:

| `gpu::ImageLayout` | Vulkan `VkImageLayout` | Metal |
|---|---|---|
| `Undefined` | `VK_IMAGE_LAYOUT_UNDEFINED` | n/a (state only) |
| `General` | `VK_IMAGE_LAYOUT_GENERAL` | n/a |
| `ColorAttachment` | `VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL` | n/a |
| `DepthAttachment` | `VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL` (or `*_DEPTH_STENCIL_*` if stencil) | n/a |
| `ShaderReadOnly` | `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL` | n/a |
| `TransferSrc` | `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL` | n/a |
| `TransferDst` | `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL` | n/a |
| `Present` | `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR` | n/a |

Metal has no layout objects; `transition()` there is bookkeeping only (update
`image->setCurrentLayout(newLayout)`), useful for validation/debug.

## `vk/CommandBuffer.hpp`

```cpp
#pragma once

#include <bg2e/gpu/CommandBuffer.hpp>
#include <bg2e/gpu/vk/common.hpp>

namespace bg2e {
namespace gpu {
namespace vk {

class Device;
class SurfaceFrame;

class CommandBuffer : public gpu::CommandBuffer {
public:
    CommandBuffer() = default;
    CommandBuffer(vk::Device* device, VkCommandBuffer cmd, VkCommandPool pool);

    void begin() override;
    void end() override;
    void transition(gpu::Image* image, ImageLayout newLayout) override;
    void beginRendering(gpu::SurfaceFrame* frame) override;
    void endRendering() override;
    void clearColor(uint32_t attachmentIndex, const gpu::Color& color) override;
    void clearDepth(float depth) override;
    bool isValid() const override { return _cmd != VK_NULL_HANDLE; }

    VkCommandBuffer handle() const { return _cmd; }

    // Used by WindowSurface::present() / Queue::submit() (step 05).
    void setPresentFrame(vk::SurfaceFrame* frame) { _presentFrame = frame; }
    vk::SurfaceFrame* presentFrame() const        { return _presentFrame; }

private:
    void flushPendingRendering(); // lazily emits vkCmdBeginRendering with recorded clears

    vk::Device*       _device = nullptr;
    VkCommandBuffer   _cmd    = VK_NULL_HANDLE;
    VkCommandPool     _pool   = VK_NULL_HANDLE;

    // recorded rendering state
    vk::SurfaceFrame* _renderFrame  = nullptr;  // frame passed to beginRendering
    bool              _renderingActive = false;
    VkClearColorValue _clearColor = {{ 0, 0, 0, 1 }};
    float             _clearDepth = 1.0f;
    bool              _hasColorClear = false;
    bool              _hasDepthClear = false;

    // present coupling (step 05)
    vk::SurfaceFrame* _presentFrame = nullptr;
};

}
}
}
```

### `vk/CommandBuffer.cpp` behavior

- **`begin()`** — `vkBeginCommandBuffer` with
  `Info::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)`.
- **`end()`** — `flushPendingRendering()` (defensive, in case `endRendering` was not called), then
  `vkEndCommandBuffer`.
- **`transition(image, newLayout)`** —
  1. `auto* vkImg = dynamic_cast<vk::Image*>(image);`
  2. `VkImageLayout oldL = toVkLayout(image->currentLayout());`
     `VkImageLayout newL = toVkLayout(newLayout);` — if equal, return.
  3. Build a `VkImageMemoryBarrier2` (use `VkDependencyInfo` + `vkCmdPipelineBarrier2`, available
     because `synchronization2` is required by `vk::Device`). Use coarse, correct masks:
     `srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT`, `dstStageMask =
     VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT`, `srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT`,
     `dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT`. Set
     `oldLayout = oldL`, `newLayout = newL`, `image = vkImg->handle()`,
     `subresourceRange.aspectMask = vkImg->aspect()`, full mip/array range.
  4. `image->setCurrentLayout(newLayout);` (allowed via the `friend` grant from step 01).
- **`beginRendering(frame)`** — `_renderFrame = dynamic_cast<vk::SurfaceFrame*>(frame);`
  `_renderingActive = true;` reset `_hasColorClear/_hasDepthClear`. Do **not** call
  `vkCmdBeginRendering` yet (lazy; see contract).
- **`clearColor(index, color)`** — store `_clearColor = { color.r, color.g, color.b, color.a }`;
  `_hasColorClear = true;` (`attachmentIndex` is honored only for index 0 in this single-color
  surface; assert/ignore others).
- **`clearDepth(depth)`** — `_clearDepth = depth; _hasDepthClear = true;`
- **`flushPendingRendering()`** — if `_renderingActive` and not yet emitted:
  build color attachment from `_renderFrame->colorImage()` (downcast to `vk::Image`, use
  `imageView()`) via `Info::attachmentInfo(view, &clearValue, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)`
  with `loadOp = CLEAR` when `_hasColorClear` else `LOAD`; build depth attachment from
  `depthImage()` via `Info::depthAttachmentInfo(view, _clearDepth)` when a depth image exists; build
  `Info::renderingInfo({w,h}, &color, depthPtr)`; call `vkCmdBeginRendering`. Mark emitted.
- **`endRendering()`** — `flushPendingRendering();` then `vkCmdEndRendering(_cmd);`
  `_renderingActive = false;`

> `Info::attachmentInfo` / `depthAttachmentInfo` / `renderingInfo` already exist (see `vk/Info.hpp`)
> and set the `loadOp/storeOp` and clear values, so no new `Info` helpers are required.

## `metal/CommandBuffer.hpp`

```cpp
#pragma once

#include <bg2e/gpu/CommandBuffer.hpp>
#include <bg2e/gpu/metal/common.hpp>

namespace bg2e {
namespace gpu {
namespace metal {

class Device;
class SurfaceFrame;

class CommandBuffer : public gpu::CommandBuffer {
public:
    CommandBuffer() = default;
#if BG2E_IS_MAC
    CommandBuffer(metal::Device* device, MTL::CommandBuffer* cmd);
#endif
    ~CommandBuffer() override;

    void begin() override;
    void end() override;
    void transition(gpu::Image* image, ImageLayout newLayout) override;
    void beginRendering(gpu::SurfaceFrame* frame) override;
    void endRendering() override;
    void clearColor(uint32_t attachmentIndex, const gpu::Color& color) override;
    void clearDepth(float depth) override;
    bool isValid() const override;

#if BG2E_IS_MAC
    MTL::CommandBuffer* handle() const { return _cmd; }
#endif

private:
#if BG2E_IS_MAC
    metal::Device*              _device     = nullptr;
    MTL::CommandBuffer*         _cmd        = nullptr;   // retained in ctor, released in dtor
    MTL::RenderPassDescriptor*  _passDesc   = nullptr;   // built in beginRendering
    MTL::RenderCommandEncoder*  _encoder    = nullptr;   // created in endRendering
    metal::SurfaceFrame*        _renderFrame = nullptr;
#endif
};

}
}
}
```

### `metal/CommandBuffer.cpp` behavior (`BG2E_IS_MAC` branch)

- **ctor** — store device, `_cmd = cmd; _cmd->retain();`
- **dtor** — release `_passDesc`, `_encoder` if still held, then `_cmd->release()`.
- **`begin()`** — no-op (Metal command buffer is ready to receive encoders); may set an internal
  "recording" flag.
- **`transition(image, newLayout)`** — bookkeeping only: `image->setCurrentLayout(newLayout);`
  (no GPU work; Metal manages hazards automatically for render targets).
- **`beginRendering(frame)`** — `_renderFrame = dynamic_cast<metal::SurfaceFrame*>(frame);`
  allocate `_passDesc = MTL::RenderPassDescriptor::alloc()->init();`
  set `colorAttachments()->object(0)->setTexture(colorTex)` from
  `_renderFrame->colorImage()` (downcast to `metal::Image`, `texture()`); if a depth image exists,
  `depthAttachment()->setTexture(depthTex)`. Default `loadAction = DontCare`, `storeAction = Store`.
  Do **not** create the encoder yet.
- **`clearColor(index, color)`** — on `_passDesc->colorAttachments()->object(0)`:
  `setLoadAction(MTL::LoadActionClear); setClearColor(MTL::ClearColor(r,g,b,a));`
- **`clearDepth(depth)`** — on `_passDesc->depthAttachment()`:
  `setLoadAction(MTL::LoadActionClear); setClearDepth(depth); setStoreAction(...)` as appropriate.
- **`endRendering()`** — `_encoder = _cmd->renderCommandEncoder(_passDesc);
  _encoder->endEncoding();` release `_encoder` and `_passDesc`, null them.
- **`end()`** — ensure no open encoder; mark ready to submit. (Actual `commit()` happens in
  `Queue::submit`, and `presentDrawable` in `Surface::present`, both later steps.)

Provide the non-`BG2E_IS_MAC` stub branch where every method throws
`std::runtime_error("Metal backend is not available on this platform")`, except trivial
`isValid()` returning `false` and a no-op destructor — matching the existing Metal files.

## `all.hpp`

Add:

```cpp
#include <bg2e/gpu/CommandBuffer.hpp>
```

## Build check

All implementations are complete but unreferenced (no `Queue` builds a `CommandBuffer` yet). New
files are auto-globbed. Build stays green on all platforms.
