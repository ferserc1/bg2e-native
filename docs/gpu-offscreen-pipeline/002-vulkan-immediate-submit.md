# Step 002 — Base `gpu::Device::immediateSubmit` + Vulkan override

**Type:** Base API + Backend (Vulkan)
**Depends on:** none
**Enables:** 005 (Vulkan image read-back), and any future high-level backend-independent code

## Goal

Add a **public, backend-agnostic** synchronous command submission primitive to the abstract
`gpu::Device`, taking a closure over the **abstract** `gpu::CommandBuffer`. It records one
command buffer, runs the closure, submits it on the graphics queue, and blocks until the GPU
finishes.

This is a general-purpose primitive intended for repeated reuse by high-level,
backend-independent code (resource uploads, one-shot layout transitions, mip generation, …),
not only by the image read-back. This step adds the abstract declaration (with a throwing
default, per the compile-safe pattern) and the **Vulkan** override; Metal is added in step 003
(until then `metal::Device` inherits the throwing default).

## Base API (`lib/include/bg2e/gpu/Device.hpp`)

```cpp
#include <functional>
// ...
class CommandBuffer;
// ...
virtual void immediateSubmit(std::function<void(CommandBuffer* cmd)>&& function)
{
    throw std::runtime_error("immediateSubmit not implemented");
}
```

The closure receives the abstract `gpu::CommandBuffer*` already in the recording state
(`begin()` is called by `immediateSubmit` before the closure and `end()` after it). The caller
records abstract commands (`cmd->transition(...)`, `cmd->beginRendering(...)`, `cmd->draw(...)`,
`cmd->dispatch(...)`, …); it must **not** call `begin()`/`end()` itself.

## Vulkan API (`lib/include/bg2e/gpu/vk/Device.hpp`)

```cpp
#include <functional>
// ...
void immediateSubmit(std::function<void(gpu::CommandBuffer* cmd)>&& function) override;
```

Supporting members (a reusable immediate pool/buffer/fence, like the stable
`render::vulkan::Command`):

```cpp
VkCommandPool   _immediateCmdPool   = VK_NULL_HANDLE;
VkCommandBuffer _immediateCmdBuffer = VK_NULL_HANDLE;
VkFence         _immediateCmdFence  = VK_NULL_HANDLE;
```

## Vulkan implementation (`lib/src/bg2e/gpu/vk/Device.cpp`)

### Create the immediate resources in `Device::create`

After `_graphicsQueue.initCommandPool(...)` (the graphics queue family index is
`indices.graphics.value()`), create a dedicated pool, a primary command buffer and an
**unsignaled** fence:

```cpp
auto poolInfo = Info::commandPoolCreateInfo(
    indices.graphics.value(),
    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
VK_ASSERT(vkCreateCommandPool(_device, &poolInfo, nullptr, &_immediateCmdPool));

auto allocInfo = Info::commandBufferAllocateInfo(_immediateCmdPool, 1);
VK_ASSERT(vkAllocateCommandBuffers(_device, &allocInfo, &_immediateCmdBuffer));

auto fenceInfo = Info::fenceCreateInfo(0); // unsignaled (reset before each use anyway)
VK_ASSERT(vkCreateFence(_device, &fenceInfo, nullptr, &_immediateCmdFence));
```

> Confirm exact `Info::fenceCreateInfo` / `commandBufferAllocateInfo` signatures in
> `lib/include/bg2e/gpu/vk/Info.hpp` (same helpers used by `vk::Queue` / `vk::WindowSurface`).

### Destroy them in `Device::cleanup`

Before destroying the VMA allocator and the device:

```cpp
if (_immediateCmdFence != VK_NULL_HANDLE) {
    vkDestroyFence(_device, _immediateCmdFence, nullptr);
    _immediateCmdFence = VK_NULL_HANDLE;
}
if (_immediateCmdPool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(_device, _immediateCmdPool, nullptr);  // also frees _immediateCmdBuffer
    _immediateCmdPool = VK_NULL_HANDLE;
    _immediateCmdBuffer = VK_NULL_HANDLE;
}
```

### The method

Wrap the reusable native command buffer in a temporary `vk::CommandBuffer` so the closure sees
the **abstract** type, then submit with the fence and wait:

```cpp
void Device::immediateSubmit(std::function<void(gpu::CommandBuffer*)>&& function)
{
    VK_ASSERT(vkResetFences(_device, 1, &_immediateCmdFence));
    VK_ASSERT(vkResetCommandBuffer(_immediateCmdBuffer, 0));

    // Temporary wrapper around the reusable native buffer. Its destructor does NOT free
    // the command buffer (vk::CommandBuffer is non-owning), so the buffer is reused.
    vk::CommandBuffer wrapper(this, _immediateCmdBuffer, _immediateCmdPool);

    wrapper.begin();          // vkBeginCommandBuffer
    function(&wrapper);       // closure records abstract commands; copy users down-cast as needed
    wrapper.end();            // flushPendingRendering() (no-op here) + vkEndCommandBuffer

    auto cmdInfo = Info::commandBufferSubmitInfo(_immediateCmdBuffer);
    auto submit  = Info::submitInfo(&cmdInfo, nullptr, nullptr);
    VK_ASSERT(queueSubmit2(_graphicsQueue.handle(), 1, &submit, _immediateCmdFence));

    VK_ASSERT(vkWaitForFences(_device, 1, &_immediateCmdFence, VK_TRUE, UINT64_MAX));
}
```

## Notes / pitfalls

- `queueSubmit2` is the extension wrapper already used by `vk::Queue::submit`
  (`#include <bg2e/gpu/vk/extensions.hpp>`); `_graphicsQueue.handle()` already exists
  (`vk::Queue::handle()`).
- `vk::CommandBuffer` has no owning destructor, so the temporary `wrapper` does not free
  `_immediateCmdBuffer` — it is reset and reused on each call. Verify this when implementing; if
  a destructor that frees is ever added, switch to constructing the wrapper once and storing it.
- `wrapper.end()` calls `flushPendingRendering()`; for a copy/transition-only buffer
  `_renderingActive` is false so it is a no-op. If a closure uses `beginRendering` without
  `endRendering`, `end()` will flush+emit — acceptable, same as a normal frame.
- Include `<bg2e/gpu/vk/CommandBuffer.hpp>` and `<bg2e/gpu/CommandBuffer.hpp>` in `Device.cpp`.

## Validation

- `bg2e` builds on all platforms. `metal::Device` inherits the throwing default until step 003.
- Nothing calls `immediateSubmit` yet; existing examples unchanged.
