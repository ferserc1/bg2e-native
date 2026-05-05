# Phase 4: Update `GPUSelectionDialog` to Use `gpu::vk::Instance`

## Objective

Replace the `render::vulkan::Instance` usage in `GPUSelectionDialog` with `gpu::vk::Instance`.
After this phase, **no production code** references `render::vulkan::Instance`.

**After this phase:** `GPUSelectionDialog` uses `gpu::vk::Instance` for GPU enumeration.
The old `render::vulkan::Instance` is completely unused and can be safely removed in Phase 5.

---

## Files to Modify

| File | Action |
|------|--------|
| `lib/src/bg2e/app/GPUSelectionDialog.cpp` | Modify — replace `render::vulkan::Instance` with `gpu::vk::Instance` |
| `lib/include/bg2e/app/GPUSelectionDialog.hpp` | Check — may need include changes |

---

## 4.1 — Update `GPUSelectionDialog.cpp`

**File:** `lib/src/bg2e/app/GPUSelectionDialog.cpp`

Replace the Instance include and usage:

```cpp
// REMOVE: #include <bg2e/render/vulkan/Instance.hpp>
#include <bg2e/gpu/vk/Instance.hpp>           // NEW

// ... other includes unchanged ...

namespace bg2e::app {

void getAvailableDevices(std::vector<std::shared_ptr<render::vulkan::PhysicalDeviceProperties>>& result)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        throw std::runtime_error("GPUSelectorDialog: Error initializing SDL");
    }

    SDL_Window* dummyWindow = SDL_CreateWindow(
        "bg2e_dummy",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        1, 1,
        SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN
    );

    if (!dummyWindow)
    {
        SDL_Quit();
        throw std::runtime_error("GPUSelectorDialog: Error creating dummy SDL Window");
    }

    gpu::vk::Instance instance;          // was: render::vulkan::Instance
    instance.create(dummyWindow);

    render::vulkan::Surface surface;
    surface.create(instance.vkInstanceHnd(), dummyWindow);  // was: surface.create(instance, dummyWindow)

    render::vulkan::PhysicalDevice::listSuitableDevices(
        instance.vkInstanceHnd(), surface, result  // was: instance, surface, result
    );
    surface.cleanup();
    instance.cleanup();

    SDL_DestroyWindow(dummyWindow);
    dummyWindow = nullptr;

    SDL_Quit();
}

// ... rest of file unchanged ...
```

**Changes summary:**
- `#include <bg2e/render/vulkan/Instance.hpp>` → `#include <bg2e/gpu/vk/Instance.hpp>`
- `render::vulkan::Instance instance` → `gpu::vk::Instance instance`
- `surface.create(instance, dummyWindow)` → `surface.create(instance.vkInstanceHnd(), dummyWindow)`
- `listSuitableDevices(instance, surface, result)` → `listSuitableDevices(instance.vkInstanceHnd(), surface, result)`

---

## 4.2 — Check `GPUSelectionDialog.hpp`

**File:** `lib/include/bg2e/app/GPUSelectionDialog.hpp`

Check if this header includes or references `render::vulkan::Instance`. If it only references
`render::vulkan::PhysicalDeviceProperties` (which it does based on the `run()` return type),
no changes are needed to the header.

---

## 4.3 — Verification

1. **Compile check:** Build the project. The dialog should compile with the new Instance type.
2. **Runtime check:** If your application uses the GPU selection dialog (multi-GPU systems),
   verify it still enumerates devices correctly and the selection UI works.
3. **grep verification:** Run `grep -r "render::vulkan::Instance" lib/` to confirm no
   production code references the old class anymore. The only remaining references should be:
   - The old `render/vulkan/Instance.hpp` and `Instance.cpp` files themselves
   - The `render/vulkan/all.hpp` aggregate include

---

## Design Decisions

| Decision | Rationale |
|----------|-----------|
| Use `gpu::vk::Instance` directly (not abstract) | GPUSelectionDialog is Vulkan-specific code (creates SDL Vulkan window, uses Vulkan PhysicalDevice). It can use the concrete type. |
| Pass `vkInstanceHnd()` to Surface/PhysicalDevice | These classes accept `VkInstance` directly (changed in Phase 2). |
