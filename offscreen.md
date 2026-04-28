# Offscreen Application API Plan

## Overview

A lightweight class that lifts only the essentials for Vulkan — **Instance → PhysicalDevice → LogicalDevice + Queues** — without SDL window, no surface, no swapchain, no loop. The structure is:

```
bg2e::app::OffscreenApplication {
    bg2e::render::vulkan::Instance     _instance;
    bg2e::render::vulkan::PhysicalDevice  _physicalDevice;
    bg2e::render::vulkan::Device         _device;

    init(config)  →  creates instance, selects GPU, creates device + queues
}
```

---

## 1. New API in `bg2e::app`

### File: `lib/include/bg2e/app/OffscreenApplication.hpp` (new)

Simple class with:
- Struct `OffscreenConfig{}` — `appId`, `width` (default 1920), `height` (default 1080)
- Method `init(const OffscreenConfig& config)` — lifts Vulkan headless (instance, GPU, device + queues)
- Method `handle() const → VkDevice&` — access to the logical device for submitting commands
- Destructor — invokes `cleanup()` (waitIdle + destroy device/instance)

### File: `lib/src/bg2e/app/OffscreenApplication.cpp` (new)

Implementation of `init()`:
1. Calls `_instance.create()` (the new overload without SDL_Window)
2. Verifies the selected device has ray tracing support if required (optional in config)
3. Calls `_physicalDevice.chooseHeadless(_instance)` (the new overload without Surface)
4. Calls `_device.create(_instance, _physicalDevice, dummySurface)` — the surface parameter is not used internally

### Update master header: `lib/include/bg2e/app/all.hpp`
- Add `#include <bg2e/app/OffscreenApplication.hpp>`

---

## 2. Modifications in `bg2e::render::vulkan::Instance`

### File: `lib/include/bg2e/render/vulkan/Instance.hpp` (modify)
- **Add new overload:** `void create()` — no parameters, for headless (no SDL)
- **Add internal overload:** `bool getRequiredExtensions(std::vector<const char*>& requiredExtensions) const` — without SDL_Window

### File: `lib/src/bg2e/render/vulkan/Instance.cpp` (modify)
- **Implement `create()` without parameters:**
  - Calls `getRequiredLayers(requiredLayers)`, same as the version with window
  - Calls new overload of `getRequiredExtensions()` without SDL
  - Builds `VkInstanceCreateInfo` normally (VK_API_VERSION_1_3, app name, etc.)
  - On macOS: adds `VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR` (same as before)
  - Creates debug messenger if debug build
  - Calls `vkCreateInstance()`

- **Implement new overload of `getRequiredExtensions()`:**
  - Instead of `SDL_Vulkan_GetInstanceExtensions(window, ...)`, filter directly from `_availableExtensions`
  - On macOS: adds `VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME`
  - If debug + validation layers: adds `VK_EXT_DEBUG_UTILS_EXTENSION_NAME`
  - Verifies all are in `_availableExtensions`, returns false if any missing
  - In release mode: returns minimal list without debugging (or with debug same, optional)

---

## 3. Modifications in `bg2e::render::vulkan::PhysicalDevice`

The core problem: `choose()`, `isSuitable()`, and `QueueFamilyIndices::get()` all take a parameter of type `Surface` as const reference and use it for:
1. Verifying queue families (present = present support)
2. Validating `SwapChainSupportDetails` (formats + present modes)

For headless we need alternatives that don't depend on Surface.

### File: `lib/include/bg2e/render/vulkan/PhysicalDevice.hpp` (modify)
- **Add to `QueueFamilyIndices`:** `static QueueFamilyIndices graphicsOnly(VkPhysicalDevice device)` — returns only queue families with VK_QUEUE_GRAPHICS_BIT, without verifying surface
- **Add method:** `void chooseHeadless(const Instance& instance)` — alternative to `choose()` without surface
- The new functions will use a `QueueFamilyIndices.isCompleteHeadless()`: both optional are non-empty (graphics is mandatory, present is optional)

### File: `lib/src/bg2e/render/vulkan/PhysicalDevice.cpp` (modify)
- **Implement `QueueFamilyIndices::get()` as wrapper to its own version:** refactor internal logic into a private helper, so that `QueueFamilyIndices::get(device, surface)` uses the normal helper and `QueueFamilyIndices::graphicsOnly(device)` uses the helper with present skip
  
- **Implement `chooseHeadless()`:** 
  - Enumerates physical devices, filters by `isSuitableHeadless(device)`
  - Query properties and score same as original, selects highest-score GPU
  - **Does not verify swapchain** — only checks device extensions (`checkDeviceExtensions` stays the same)
  - Stores `_device`, `_properties`. **Does NOT store surface pointer** (`_surface` stays nullptr)
  
- **Add `isSuitableHeadless(VkPhysicalDevice device)` as static:** same as `isSuitable()` but:
  - Verifies queue families with `graphicsOnly()`  
  - Checks device extensions (same)
  - **SKIPS** swapchain support check
  
- **Modify `queueFamilyIndices() const`:** if `_surface == nullptr`, returns a structure where only graphics is present (calls `graphicsOnly(handle())`)

---

## 4. No modifications needed in:

- **`Device::create()`** → Already takes `const Surface& surface` but the parameter is not used inside the implementation (in Device.cpp line 30: `const Surface&`). A dummy object can be passed.
- **`Surface`** → Not needed and not used in the headless path.

---

## 5. Flow Diagram

```
App user code:
    ┌────────────────────────┐
    │ OffscreenApplication   │
    │ app.init(config)       │
    └────────┬───────────────┘
             │
     ┌───────▼─────────────────────────────┐
     │ _instance.create() [no-SDL version] │──► vkCreateInstance() 1.3
     └───────┬─────────────────────────────┘
             │
     ┌───────▼─────────────────────────────┐
     │ _physicalDevice                     │
     │   .chooseHeadless(_instance)        │──► vkEnumeratePhysicalDevices()
     │   • graphicsOnly(queue families)    │──► query device properties
     │   • skip swapchain check            │──► select highest-score GPU
     └───────┬─────────────────────────────┘
             │
     ┌───────▼─────────────┐  (surface param unused)
     │ _device.create(...) │──► vkCreateDevice() → features chain → enabled extensions
     │  + queue fetch      │──► vkGetDeviceQueue(graphics)
     └─────────────────────┘

    // Now app.handle() returns a valid VkDevice
    // app.graphicsQueue() returns a valid graphics queue

    // User renders directly with Vulkan: create command buffers,
    // submit to queue, wait idle...

     ┌───────▼────────────────────┐
     │ destructor: cleanup()      │──► _device.waitIdle()
     │   (automatic or explicit)  │──► vkDestroyDevice(_device)
     │                           │──► _instance.cleanup() (debug messenger + vkDestroyInstance)
     └────────────────────────────┘
```

---

## Summary of changes by file

| File | Type | Change |
|------|------|--------|
| `lib/include/bg2e/app/OffscreenApplication.hpp` | **New** | Declaration of `OffscreenConfig` and `OffscreenApplication` |
| `lib/src/bg2e/app/OffscreenApplication.cpp` | **New** | Implementation of `init()` calling vulkan:: API without SDL/surface, destructor with cleanup |
| `lib/include/bg2e/app/all.hpp` | Modify | `#include <bg2e/app/OffscreenApplication.hpp>` |
| `lib/include/bg2e/render/vulkan/Instance.hpp` | Modify | `void create()`, new overload of `getRequiredExtensions()` without SDL_Window* |
| `lib/src/bg2e/render/vulkan/Instance.cpp` | Modify | Implementation of both new functions, direct filtering from `_availableExtensions` |
| `lib/include/bg2e/render/vulkan/PhysicalDevice.hpp` | Modify | `chooseHeadless()`, `QueueFamilyIndices::graphicsOnly()` |
| `lib/src/bg2e/render/vulkan/PhysicalDevice.cpp` | Modify | Implementation of both functions, refactor `QueueFamilyIndices::get()` into internal helper |
