# Step 02 — Command Queue Changes

## Overview

Modify `Command` to store and use the dedicated immediate queue. The `_immediateCmdPool` remains created on `_graphicsQueueFamily` (which is correct — pools are family-scoped), but submissions go to `_immediateQueue`.

## Files to Modify

### `lib/include/bg2e/render/vulkan/Command.hpp`

Add the immediate queue member. The header currently does not store the immediate queue — it retrieves it indirectly through `_engine`. We store it for clarity and to avoid repeated getter calls.

```cpp
protected:
    VkQueue _graphicsQueue = VK_NULL_HANDLE;
    VkQueue _immediateQueue = VK_NULL_HANDLE;  // NEW: dedicated queue for immediateSubmit
    uint32_t _graphicsQueueFamily = 0;

    VkCommandPool _immediateCmdPool = VK_NULL_HANDLE;
    VkCommandBuffer _immediateCmdBuffer = VK_NULL_HANDLE;
    VkFence _immediateCmdFence = VK_NULL_HANDLE;

    Engine * _engine;
```

No new public methods needed. The `graphicsQueue()` and `graphicsQueueFamily()` getters remain unchanged.

### `lib/src/bg2e/render/vulkan/Command.cpp`

**Change 1**: In `Command::init()`, retrieve the immediate queue from the device:

```cpp
void Command::init(Engine *engine)
{
    _engine = engine;
    
    _graphicsQueue = engine->device().graphicsQueue();
    _graphicsQueueFamily = engine->device().graphicsFamily();
    _immediateQueue = engine->device().immediateQueue();  // NEW

    auto cmdPoolInfo = Info::commandPoolCreateInfo(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_ASSERT(vkCreateCommandPool(_engine->device().handle(), &cmdPoolInfo, nullptr, &_immediateCmdPool));
    
    auto cmdAllocInfo = Info::commandBufferAllocateInfo(_immediateCmdPool);
    VK_ASSERT(vkAllocateCommandBuffers(_engine->device().handle(), &cmdAllocInfo, &_immediateCmdBuffer));
    
    auto fenceInfo = Info::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_ASSERT(vkCreateFence(_engine->device().handle(), &fenceInfo, nullptr, &_immediateCmdFence));
    _engine->cleanupManager().push([&](VkDevice) {
        vkDestroyCommandPool(_engine->device().handle(), _immediateCmdPool, nullptr);
        vkDestroyFence(_engine->device().handle(), _immediateCmdFence, nullptr);
    });
}
```

**Change 2**: In `Command::immediateSubmit()`, replace `_graphicsQueue` with `_immediateQueue` in the submission call:

```cpp
void Command::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
{
    VK_ASSERT(vkResetFences(_engine->device().handle(), 1, &_immediateCmdFence));
    VK_ASSERT(vkResetCommandBuffer(_immediateCmdBuffer, 0));

    VkCommandBufferBeginInfo cmdBeginInfo = Info::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_ASSERT(vkBeginCommandBuffer(_immediateCmdBuffer, &cmdBeginInfo));

    function(_immediateCmdBuffer);

    VK_ASSERT(vkEndCommandBuffer(_immediateCmdBuffer));

    auto cmdInfo = Info::commandBufferSubmitInfo(_immediateCmdBuffer);
    auto submit = Info::submitInfo(&cmdInfo, nullptr, nullptr);

    // Submit to the dedicated immediate queue, not the graphics queue
    queueSubmit2(_immediateQueue, 1, &submit, _immediateCmdFence);

    VK_ASSERT(vkWaitForFences(_engine->device().handle(), 1, &_immediateCmdFence, true, 9999999999));
}
```

**No other changes needed**. The `createCommandPool()` method remains unchanged — it continues to create pools on `_graphicsQueueFamily`, which is correct since those pools are used by `FrameResources` and `UserInterface` on the main thread.

## Integration Points

- `Command::init()` is called from `Engine::createDevicesAndQueues()` at `lib/src/bg2e/render/Engine.cpp:208`. By this point `_device.create()` has already run, so `_immediateQueue` is valid.
- All existing callers of `immediateSubmit()` throughout the codebase (Image uploads, Mesh creation, CubemapRenderer, GPUProcess, SelectionManager, Swapchain, TemporalAccumulator, RayTracingMesh) are unaffected — the function signature is unchanged, only the internal submission queue changes.
- `FrameResources::init()` calls `command->createCommandPool()`, which uses `_graphicsQueueFamily`. This is correct and unchanged.

## Callers of immediateSubmit (for reference)

These files call `engine->command().immediateSubmit(...)` and will automatically benefit from the queue isolation:

| File | Purpose |
|------|---------|
| `lib/src/bg2e/render/vulkan/Image.cpp` | Image layout transitions, staging buffer copies |
| `lib/src/bg2e/render/vulkan/geo/Mesh.cpp` | Vertex/index buffer uploads |
| `lib/src/bg2e/render/vulkan/rt/RayTracingMesh.cpp` | BLAS builds |
| `lib/src/bg2e/render/vulkan/Swapchain.cpp` | Swapchain image transitions |
| `lib/src/bg2e/render/CubemapRenderer.cpp` | Cubemap rendering |
| `lib/src/bg2e/render/GPUProcess.cpp` | Compute shader dispatch (LUT generation) |
| `lib/src/bg2e/render/deferred/TemporalAccumulator.cpp` | Temporal accumulation |
| `lib/src/bg2e/manipulation/SelectionManager.cpp` | Selection highlight textures |
| `examples/02_compute_shader/src/main.cpp` | Compute shader example |

## Validation

After this step:

1. Run any example (e.g., `01_setup`) — should render normally with no validation errors.
2. Run with async scene loading enabled — background thread submissions go to `_immediateQueue`, main loop submissions go to `_graphicsQueue`. No queue contention.
3. Vulkan validation should show no `UNASSIGNED-Threading-MultipleThreads` or similar errors.
