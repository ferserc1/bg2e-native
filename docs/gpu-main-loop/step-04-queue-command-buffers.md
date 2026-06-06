# Step 4 — `Queue::createCommandBuffer()` + `Queue::submit()` (plain submit)

**Goal:** let a `Queue` produce `CommandBuffer`s and submit them. This step implements a **plain**
submit (graphics/transfer work, no swapchain presentation). Per-frame present coupling and
synchronization are added in step 05.

Adding the two pure virtuals to `gpu::Queue` forces **both** `vk::Queue` and `metal::Queue` to
implement them in this same step, so the build never breaks.

Depends on: step 03 (`CommandBuffer`).

## Files

- **Modify** `lib/include/bg2e/gpu/Queue.hpp`
- **Modify** `lib/include/bg2e/gpu/vk/Queue.hpp`
- **Modify** `lib/src/bg2e/gpu/vk/Queue.cpp`
- **Modify** `lib/src/bg2e/gpu/vk/Device.cpp` (create/destroy command pools for the queues)
- **Modify** `lib/include/bg2e/gpu/metal/Queue.hpp`
- **Modify** `lib/src/bg2e/gpu/metal/Queue.cpp`

## `gpu::Queue` additions (abstract)

The loop holds `const Queue&`, so both methods are `const`.

```cpp
#include <memory>

namespace bg2e {
namespace gpu {

class CommandBuffer;

class BG2E_API Queue {
public:
    virtual ~Queue() = default;
    virtual uint32_t familyIndex() const = 0;
    virtual bool isValid() const = 0;

    virtual std::shared_ptr<gpu::CommandBuffer> createCommandBuffer() const = 0;
    virtual void submit(gpu::CommandBuffer* cmd) const = 0;
};

}
}
```

## Vulkan: command pool ownership

`vk::Queue` needs a `VkCommandPool` to allocate command buffers from. The pool is created when the
device is created (non-`const` context) and destroyed before the device. `vk::Queue` is stored by
value in `vk::Device` and handed out as `const Queue&`, so allocation must work through a `const`
method using a pool created earlier.

### `vk/Queue.hpp` changes

```cpp
class Queue : public gpu::Queue {
public:
    Queue() = default;
    Queue(VkQueue queue, uint32_t family);

    uint32_t familyIndex() const override;
    bool isValid() const override;

    VkQueue handle() const;

    // Pool lifecycle — called from vk::Device::create / cleanup.
    void initCommandPool(VkDevice device);
    void destroyCommandPool();

    std::shared_ptr<gpu::CommandBuffer> createCommandBuffer() const override;
    void submit(gpu::CommandBuffer* cmd) const override;

private:
    VkQueue       _queue{VK_NULL_HANDLE};
    uint32_t      _familyIndex{UINT32_MAX};
    VkDevice      _device{VK_NULL_HANDLE};     // borrowed
    VkCommandPool _commandPool{VK_NULL_HANDLE};
};
```

### `vk/Queue.cpp` changes

- **`initCommandPool(device)`** — store `_device = device`; create the pool with
  `Info::commandPoolCreateInfo(_familyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)` and
  `vkCreateCommandPool`. (`Info::commandPoolCreateInfo` already exists.)
- **`destroyCommandPool()`** — `vkDestroyCommandPool(_device, _commandPool, nullptr)` if set; null
  both. Must be called *before* `vkDestroyDevice`.
- **`createCommandBuffer() const`** — allocate one primary buffer via
  `Info::commandBufferAllocateInfo(_commandPool, 1)` + `vkAllocateCommandBuffers`; wrap it in a
  `std::make_shared<vk::CommandBuffer>(/* device */, vkCmd, _commandPool)`. Allocation does not
  mutate `vk::Queue`'s own members, so `const` is honored (the pool handle is used, not changed).
  > Needs a `vk::Device*`. Either store a `vk::Device*` in the queue (set in `initCommandPool` /
  > `create`) or pass the `VkDevice` + `VmaAllocator` the `vk::CommandBuffer` needs. Simplest:
  > give `vk::Queue` a `vk::Device*` borrowed pointer set when the pool is initialized, and pass it
  > to the command-buffer constructor.
- **`submit(cmd) const`** — plain submit for this step:
  - `auto* vkCmd = dynamic_cast<vk::CommandBuffer*>(cmd);`
  - Build `VkSubmitInfo2` with a single `VkCommandBufferSubmitInfo` (use
    `Info::commandBufferSubmitInfo(vkCmd->handle())` and `Info::submitInfo(&cmdInfo, nullptr,
    nullptr)`), no wait/signal semaphores, no fence.
  - `vkQueueSubmit2(_queue, 1, &submit, VK_NULL_HANDLE);`
  - (Step 05 replaces this body with the semaphore/fence + present-aware version when
    `vkCmd->presentFrame()` is set.)

### `vk/Device.cpp` changes

After the queues are assigned in `Device::create`, initialize their pools:

```cpp
_graphicsQueue.initCommandPool(_device);
if (!offscreen) _presentQueue.initCommandPool(_device);
_transferQueue.initCommandPool(_device);
```

In `Device::cleanup`, destroy the pools **before** `vkDestroyDevice` (and before
`vmaDestroyAllocator`, ordering with VMA is independent):

```cpp
_graphicsQueue.destroyCommandPool();
_presentQueue.destroyCommandPool();
_transferQueue.destroyCommandPool();
// ... existing vmaDestroyAllocator + vkDestroyDevice ...
```

> `vk::Queue` currently has no move/cleanup ownership of the pool; because it is stored by value
> and reassigned with `_graphicsQueue = vk::Queue(...)`, set the pool **after** that assignment
> (as above), never inside the temporary.

## Metal: command buffers from the command queue

`metal::Queue` already wraps an `MTL::CommandQueue`. No pool concept.

### `metal/Queue.hpp` changes

```cpp
std::shared_ptr<gpu::CommandBuffer> createCommandBuffer() const override;
void submit(gpu::CommandBuffer* cmd) const override;
```

(plus, in the non-mac stub region, the same overrides).

### `metal/Queue.cpp` changes (`BG2E_IS_MAC`)

- **`createCommandBuffer() const`** —
  `MTL::CommandBuffer* mtlCmd = _commandQueue->commandBuffer();` (autoreleased) and wrap:
  `return std::make_shared<metal::CommandBuffer>(/* device */, mtlCmd);` The `metal::CommandBuffer`
  ctor retains it.
  > Needs a `metal::Device*`. Store a borrowed `metal::Device*` in `metal::Queue` (set when the
  > device creates the queue in `metal::Device::create`), or pass it down. Add a
  > `void setDevice(metal::Device*)` used from `Device::create`, mirroring the Vulkan approach.
- **`submit(cmd) const`** — plain submit for this step:
  `auto* mtlCmd = dynamic_cast<metal::CommandBuffer*>(cmd); mtlCmd->handle()->commit();`
  (Step 05 adds the `presentDrawable` ordering via `Surface::present`, which runs *before*
  `submit`, so `commit()` here already presents correctly once present is wired.)

Non-mac stubs throw `std::runtime_error("Metal backend is not available on this platform")`.

> `metal::Device::create` already builds the three `metal::Queue`s; add `q.setDevice(this);` after
> each assignment so the queues can create command buffers. This is a `bg2e::gpu` change only.

## Build check

`gpu::Queue` gains two pure virtuals; the only subclasses (`vk::Queue`, `metal::Queue`) implement
them here. `Device` pool wiring is internal to the `gpu` layer. Existing example `02_device` still
only queries the queue (no new calls), so it keeps compiling and running. Build stays green.
