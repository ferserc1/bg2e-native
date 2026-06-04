# Step 4 — VMA allocator in `gpu::vk::Device`

**Goal:** Give the Vulkan logical device a `VmaAllocator` so `vk::Image` (Step 5) can allocate
offscreen color/depth images. The allocator is created at the end of `Device::create` and
destroyed at the start of `Device::cleanup`. Nothing consumes it yet → build stays green.

## Files

- **Modify** `lib/include/bg2e/gpu/vk/Device.hpp`
- **Modify** `lib/src/bg2e/gpu/vk/Device.cpp`

## `vk/Device.hpp`

Add the member + getter (VMA types come from `vk/common.hpp`, already included):

```cpp
class Device : public gpu::Device {
public:
    // ... existing ...
    VkDevice     handle() const { return _device; }
    VmaAllocator allocator() const { return _allocator; }

private:
    VkDevice _device{VK_NULL_HANDLE};
    VmaAllocator _allocator{VK_NULL_HANDLE};
    vk::Queue _graphicsQueue;
    vk::Queue _presentQueue;
    vk::Queue _transferQueue;
};
```

## `vk/Device.cpp`

### In `Device::create`, after `vkCreateDevice` succeeds and queues are retrieved

```cpp
// --- VMA allocator ---
auto* vkInst = dynamic_cast<vk::Instance*>(instance);

VmaAllocatorCreateInfo allocatorInfo{};
allocatorInfo.physicalDevice = vkPhysDevice->handle();
allocatorInfo.device         = _device;
allocatorInfo.instance       = vkInst->vkInstanceHnd();
// Match the device feature set enabled above (bufferDeviceAddress = VK_TRUE).
allocatorInfo.flags          = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

VK_ASSERT(vmaCreateAllocator(&allocatorInfo, &_allocator));
```

> `instance` is already a parameter of `Device::create`; cast it to `vk::Instance` to obtain
> the `VkInstance`. `vkPhysDevice` is the existing local cast of the physical device.
> `<bg2e/gpu/vk/Instance.hpp>` is already included by `Device.cpp`.

### In `Device::cleanup`, before `vkDestroyDevice`

```cpp
void Device::cleanup()
{
    if (_allocator != VK_NULL_HANDLE)
    {
        vmaDestroyAllocator(_allocator);
        _allocator = VK_NULL_HANDLE;
    }
    if (_device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(_device, nullptr);
        _device = VK_NULL_HANDLE;
    }
}
```

## Notes

- VMA's implementation (`VMA_IMPLEMENTATION`) must be compiled in exactly one TU. The `gpu`
  framework already links VMA (it's used elsewhere in the project's build). **Do not** add a
  second `VMA_IMPLEMENTATION` definition — only call the API here. If the gpu framework turns
  out not to already provide the VMA implementation TU, that is a build-config concern to raise
  with the user (no CMake edits per project rules).
- `bufferDeviceAddress` is enabled in the existing feature chain, so the matching allocator
  flag keeps VMA consistent with the device.

## Compile check

Allocator created/destroyed but unused. Examples 02/03 still run identically. Build stays green.
