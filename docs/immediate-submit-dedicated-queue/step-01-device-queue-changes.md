# Step 01 — Device Queue Changes

## Overview

Modify `Device` to request two graphics queues at device creation and expose the second one via `immediateQueue()`.

## Files to Modify

### `lib/include/bg2e/render/vulkan/Device.hpp`

Add a new member and getter for the immediate queue:

```cpp
// After _presentQueue member (line 50)
VkQueue _immediateQueue = VK_NULL_HANDLE;

// After presentQueue() getter (line 41)
inline VkQueue immediateQueue() const { return _immediateQueue; }
```

Full updated class interface:

```cpp
class BG2E_API Device {
public:
    void create(VkInstance instance, const PhysicalDevice& physicalDevice, bool offscreen);
    void cleanup();

    inline VkDevice handle() const { return _device; }
    inline bool isValid() const { return _device != VK_NULL_HANDLE; }

    inline VkQueue graphicsQueue() const { return _graphicsQueue; }
    inline VkQueue presentQueue() const { return _presentQueue; }
    inline VkQueue immediateQueue() const { return _immediateQueue; }
    inline uint32_t graphicsFamily() const { return _graphicsFamily; }
    inline uint32_t presentFamily() const { return _presentFamily; }

    void waitIdle() const;

protected:
    VkDevice _device = VK_NULL_HANDLE;
    VkQueue _graphicsQueue = VK_NULL_HANDLE;
    VkQueue _presentQueue = VK_NULL_HANDLE;
    VkQueue _immediateQueue = VK_NULL_HANDLE;
    uint32_t _graphicsFamily = 0;
    uint32_t _presentFamily = 0;
};
```

### `lib/src/bg2e/render/vulkan/Device.cpp`

**Change 1**: In `Device::create()`, request 2 queues for the graphics family instead of 1.

At line 49, change:
```cpp
queueCreateInfo.queueCount = 1;
```
to:
```cpp
queueCreateInfo.queueCount = 2;
```

**Change 2**: After retrieving `_graphicsQueue` at line 191, also retrieve `_immediateQueue`:

```cpp
_graphicsFamily = indices.graphics.value();
vkGetDeviceQueue(_device, _graphicsFamily, 0, &_graphicsQueue);
vkGetDeviceQueue(_device, _graphicsFamily, 1, &_immediateQueue);

if (!offscreen)
{
    _presentFamily = indices.present.value();
    vkGetDeviceQueue(_device, _presentFamily, 0, &_presentQueue);
}
```

## Integration Points

- `Engine::createDevicesAndQueues()` calls `_device.create(...)` followed by `_command.init(this)`. The `_immediateQueue` is available after `_device.create()` returns, so `Command::init()` can access it.
- No other code in the engine needs to change for this step. The new queue is only consumed by `Command::immediateSubmit()` in Step 02.

## Validation

After this step, running with Vulkan validation layers should show no errors related to queue creation. The device creation succeeds with 2 graphics queues. If the hardware only supports 1 queue in the graphics family, `vkGetDeviceQueue` with index 1 returns `VK_NULL_HANDLE` gracefully (though this is extremely rare — most GPUs support at least 2).

To verify: run any existing example and confirm no validation errors appear at device creation.
