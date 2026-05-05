# Phase 3: Update `Engine` to Use `gpu::Instance`

## Objective

Replace `render::vulkan::Instance` with `gpu::vk::Instance` inside `render::Engine`. Engine's
public API exposes `gpu::Instance*` (abstract interface), while internally it holds a
`gpu::vk::Instance` for Vulkan-specific operations (accessing `vkInstanceHnd()`).

**After this phase:** Engine no longer depends on `render::vulkan::Instance`. The public API
returns `gpu::Instance*`. Internal Vulkan calls use `gpu::vk::Instance::vkInstanceHnd()`.
The old `render::vulkan::Instance` class is now unused by any production code.

---

## Files to Modify

| File | Action |
|------|--------|
| `lib/include/bg2e/render/Engine.hpp` | Modify — replace Instance member type and public accessor |
| `lib/src/bg2e/render/Engine.cpp` | Modify — use `gpu::vk::Instance` internally |

---

## 3.1 — Update Engine Header

**File:** `lib/include/bg2e/render/Engine.hpp`

Replace the `render::vulkan::Instance` include and member with the new gpu types:

```cpp
#pragma once

#include <bg2e/common.hpp>
// REMOVE: #include <bg2e/render/vulkan/Instance.hpp>
#include <bg2e/gpu/Instance.hpp>              // NEW: abstract interface
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/Command.hpp>
#include <bg2e/render/vulkan/Swapchain.hpp>
#include <bg2e/render/vulkan/extensions.hpp>
#include <bg2e/render/vulkan/CleanupManager.hpp>
#include <bg2e/render/vulkan/FrameResources.hpp>
#include <bg2e/render/vulkan/Surface.hpp>
#include <bg2e/render/vulkan/PhysicalDevice.hpp>
#include <bg2e/render/vulkan/Device.hpp>

#include <memory>  // for std::unique_ptr

namespace bg2e {

namespace gpu::vk {
class Instance;  // forward declaration for internal use
}

namespace render {

namespace vulkan {
class DescriptorSetAllocator;
}

class BG2E_API Engine {
public:
    void init(SDL_Window* windowPtr);
    void init();  // Offscreen

    void cleanup();

    // Public API returns abstract interface
    inline gpu::Instance* instance() { return _gpuInstance.get(); }
    inline const gpu::Instance* instance() const { return _gpuInstance.get(); }

    // ... all other public methods unchanged ...

    inline const vulkan::PhysicalDevice& physicalDevice() const { return _physicalDevice; }
    const vulkan::Surface& surface() const;
    inline const vulkan::Device& device() const { return _device; }
    vulkan::Swapchain& swapchain();
    const vulkan::Swapchain& swapchain() const;
    inline vulkan::Command& command() { return _command; }
    inline const vulkan::Command& command() const { return _command; }
    // ... rest of public API unchanged ...

protected:
    SDL_Window* _windowPtr = nullptr;

private:
    bool _debugLayers = true;

    // NEW: Instance ownership via gpu::vk::Instance
    std::unique_ptr<gpu::vk::Instance> _gpuInstance;

    // Vulkan-specific internal accessor
    VkInstance vkInstance() const;

    vulkan::Surface _surface;
    vulkan::PhysicalDevice _physicalDevice;
    vulkan::Device _device;
    vulkan::Swapchain _swapchain;
    vulkan::Command _command;

    std::unique_ptr<vulkan::DescriptorSetAllocator> _descriptorSetAllocator;
    std::vector<vulkan::FrameResources> _frameResources;
    uint32_t _currentFrame = 0;

    vulkan::CleanupManager _cleanupManager;
    VmaAllocator _allocator = VK_NULL_HANDLE;
    bool _resizeRequested = false;

    void createInstance();
    void createSurface();
    void createDevicesAndQueues();
    void createMemoryAllocator();
    void createFrameResources();
    void cleanupFrameResources();
};

}
}

// Resolve forward declaration
#include <bg2e/render/vulkan/DescriptorSetAllocator.hpp>
```

**Key changes:**
- Removed `#include <bg2e/render/vulkan/Instance.hpp>`
- Added `#include <bg2e/gpu/Instance.hpp>` and forward declaration of `gpu::vk::Instance`
- Member `_instance` (value) → `_gpuInstance` (`std::unique_ptr<gpu::vk::Instance>`)
- Public accessor returns `gpu::Instance*` instead of `const vulkan::Instance&`
- Added private `VkInstance vkInstance() const` helper for internal Vulkan calls
- Added `#include <memory>` for `std::unique_ptr`

---

## 3.2 — Update Engine Source

**File:** `lib/src/bg2e/render/Engine.cpp`

Replace all Instance usage:

```cpp
#include <bg2e/render/Engine.hpp>
#include <bg2e/gpu/vk/Instance.hpp>           // NEW: concrete Vulkan implementation
#include <bg2e/render/vulkan/extensions.hpp>
#include <stdexcept>

// ... SDL includes unchanged ...

namespace bg2e {
namespace render {

// Internal helper: gets VkInstance from the concrete implementation
VkInstance Engine::vkInstance() const
{
    return _gpuInstance->vkInstanceHnd();
}

void Engine::init(SDL_Window* windowPtr)
{
    _windowPtr = windowPtr;

    int width = 0;
    int height = 0;
    SDL_GetWindowSize(_windowPtr, &width, &height);

    createInstance();
    vulkan::loadInstanceExtensions(vkInstance(), false);  // was: _instance.handle()
    createSurface();
    createDevicesAndQueues();
    createMemoryAllocator();

    vulkan::loadDeviceExtensions(_physicalDevice, _device.handle(), false);

    _swapchain.init(this, uint32_t(width), uint32_t(height));

    createFrameResources();

    _descriptorSetAllocator = std::unique_ptr<vulkan::DescriptorSetAllocator>(
        new vulkan::DescriptorSetAllocator()
    );
    _descriptorSetAllocator->init(this);
    _cleanupManager.push([&](VkDevice) {
        _descriptorSetAllocator.reset();
    });
}

void Engine::init()
{
    createInstance();
    vulkan::loadInstanceExtensions(vkInstance(), true);  // was: _instance.handle()
    createDevicesAndQueues();
    createMemoryAllocator();

    vulkan::loadDeviceExtensions(_physicalDevice, _device.handle(), true);

    createFrameResources();

    _descriptorSetAllocator = std::make_unique<vulkan::DescriptorSetAllocator>();
    _descriptorSetAllocator->init(this);
    _cleanupManager.push([&](VkDevice) {
        _descriptorSetAllocator.reset();
    });
}

void Engine::cleanup()
{
    _device.waitIdle();
    _cleanupManager.flush(_device);
    cleanupFrameResources();
    _swapchain.cleanup();
    vmaDestroyAllocator(_allocator);
    _device.cleanup();
    _surface.cleanup();
    _gpuInstance->cleanup();  // was: _instance.cleanup()
}

// ... surface(), swapchain(), newFrame() unchanged ...

void Engine::createInstance()
{
    _gpuInstance = std::make_unique<gpu::vk::Instance>();
    if (isOffscreen())
    {
        _gpuInstance->create();
    }
    else
    {
        _gpuInstance->create(_windowPtr);
    }
}

void Engine::createSurface()
{
    _surface.create(vkInstance(), _windowPtr);  // was: _surface.create(_instance, _windowPtr)
}

void Engine::createDevicesAndQueues()
{
    if (isOffscreen())
    {
        _physicalDevice.choose(vkInstance());  // was: _physicalDevice.choose(_instance)
    }
    else
    {
        _physicalDevice.choose(vkInstance(), _surface);  // was: _physicalDevice.choose(_instance, _surface)
    }

    _device.create(vkInstance(), _physicalDevice, isOffscreen());
    // was: _device.create(_instance, _physicalDevice, isOffscreen())
    _command.init(this);
}

void Engine::createMemoryAllocator()
{
    VmaAllocatorCreateInfo allocInfo = {};
    allocInfo.physicalDevice = _physicalDevice.handle();
    allocInfo.device = _device.handle();
    allocInfo.instance = vkInstance();  // was: _instance.handle()
    allocInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocInfo, &_allocator);
}

// ... rest of file unchanged ...
}
}
```

**Summary of changes in Engine.cpp:**
- Added `#include <bg2e/gpu/vk/Instance.hpp>`
- Added `vkInstance()` helper method
- `createInstance()` creates `gpu::vk::Instance` via `make_unique`
- All `_instance.handle()` → `vkInstance()`
- All `_instance.cleanup()` → `_gpuInstance->cleanup()`
- All `_instance.create(...)` → `_gpuInstance->create(...)`
- Pass `vkInstance()` to Surface, PhysicalDevice, Device

---

## 3.3 — Update `Engine.hpp` Instance Accessor Signature

The old accessor was:
```cpp
inline const vulkan::Instance& instance() const { return _instance; }
```

The new accessor is:
```cpp
inline gpu::Instance* instance() { return _gpuInstance.get(); }
inline const gpu::Instance* instance() const { return _gpuInstance.get(); }
```

**Breaking change for callers:** Any code that called `engine.instance().handle()` will no
longer compile. These callers must be updated to use the concrete Vulkan type or receive the
VkInstance handle through other means.

**Search for callers of `engine.instance()`:**
The primary callers after Phase 2 are already updated (Surface, PhysicalDevice, Device now
take VkInstance directly). Any remaining callers of `engine.instance()` in user code
(renderers, scene, etc.) need to be audited.

---

## 3.4 — Audit Remaining `engine.instance()` Callers

Search the codebase for any remaining uses of `engine.instance()` or `_engine->instance()`.
Common patterns to look for:

```
engine.instance().handle()
_engine->instance().handle()
```

These will break because the new accessor returns `gpu::Instance*` which has no `handle()`.
Options for fixing:
1. If the caller needs `VkInstance`, it should receive it through a different path
   (e.g., stored from init time, or accessed through a Vulkan-specific helper)
2. If the caller is Vulkan-specific code that can safely downcast, use:
   ```cpp
   static_cast<gpu::vk::Instance*>(engine.instance())->vkInstanceHnd()
   ```

---

## 3.5 — Verification

1. **Compile check:** Build the project. Fix any remaining callers of `engine.instance().handle()`.
2. **Runtime check:** Run the engine. Verify:
   - Instance creation works for both windowed and offscreen modes
   - Validation layers appear in debug builds
   - Surface, device, swapchain all initialize correctly
   - VMA allocator creates successfully
3. **No behavior change:** The logic is identical, only the ownership and type changed.

---

## Design Decisions

| Decision | Rationale |
|----------|-----------|
| `std::unique_ptr<gpu::vk::Instance>` ownership | Instance is now heap-allocated through the gpu abstraction, matching the pattern used by `gpu::vk::Backend`. |
| `VkInstance vkInstance()` private helper | Centralizes the downcast to one place. All internal Vulkan calls go through this helper. |
| Public API returns `gpu::Instance*` (pointer) | Follows the "Abstract only" strategy chosen by the user. Pointer allows polymorphism. |
| Forward declaration of `gpu::vk::Instance` | Avoids including the concrete header in the Engine header. Only the .cpp includes it. |
