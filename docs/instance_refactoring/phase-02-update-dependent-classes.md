# Phase 2: Update `render::vulkan` Dependent Classes

## Objective

Modify `Surface`, `PhysicalDevice`, and `Device` (in `render::vulkan` namespace) to accept
`VkInstance` directly instead of `const Instance&`. This decouples them from the Instance
class entirely — they only need the raw Vulkan handle, not the wrapper.

**After this phase:** Surface, PhysicalDevice, and Device no longer reference
`render::vulkan::Instance` in their API. They accept `VkInstance` directly. The old
`render::vulkan::Instance` still exists but is only used by Engine and GPUSelectionDialog.

---

## Files to Modify

| File | Action |
|------|--------|
| `lib/include/bg2e/render/vulkan/Surface.hpp` | Modify — change `create()` signature |
| `lib/src/bg2e/render/vulkan/Surface.cpp` | Modify — use `VkInstance` directly |
| `lib/include/bg2e/render/vulkan/PhysicalDevice.hpp` | Modify — change `choose()` and `listSuitableDevices()` signatures |
| `lib/src/bg2e/render/vulkan/PhysicalDevice.cpp` | Modify — use `VkInstance` directly |
| `lib/include/bg2e/render/vulkan/Device.hpp` | Modify — change `create()` signature |
| `lib/src/bg2e/render/vulkan/Device.cpp` | Modify — use `VkInstance` directly, inline layer query |
| `lib/src/bg2e/render/Engine.cpp` | Modify — pass `_instance.handle()` instead of `_instance` |
| `lib/src/bg2e/app/GPUSelectionDialog.cpp` | Modify — pass `instance.handle()` instead of `instance` |

---

## 2.1 — Update `Surface`

### Header: `lib/include/bg2e/render/vulkan/Surface.hpp`

Remove the forward declaration of `class Instance` and change the `create()` signature:

```cpp
// REMOVE: class Instance;

class BG2E_API Surface {
public:
    void create(VkInstance instance, SDL_Window* window);  // was: const Instance&
    void cleanup();
    // ... rest unchanged
};
```

### Source: `lib/src/bg2e/render/vulkan/Surface.cpp`

```cpp
// REMOVE: #include <bg2e/render/vulkan/Instance.hpp>

void Surface::create(VkInstance instance, SDL_Window* window)
{
    _window = window;
    _instance = instance;  // was: instance.handle()
    SDL_Vulkan_CreateSurface(window, instance, &_surface);  // was: instance.handle()
}
```

The rest of the file remains unchanged.

---

## 2.2 — Update `PhysicalDevice`

### Header: `lib/include/bg2e/render/vulkan/PhysicalDevice.hpp`

Remove the forward declaration and update all methods that take `const Instance&`:

```cpp
// REMOVE: class Instance;

class PhysicalDevice {
public:
    static void listSuitableDevices(
        VkInstance instance,              // was: const Instance&
        const Surface& surface,
        std::vector<std::shared_ptr<PhysicalDeviceProperties>>& result
    );

    void choose(VkInstance instance, const Surface& surface);  // was: const Instance&
    void choose(VkInstance instance);                           // was: const Instance&
    // ... rest unchanged
};
```

### Source: `lib/src/bg2e/render/vulkan/PhysicalDevice.cpp`

All occurrences of `instance.handle()` become just `instance`:

```cpp
// REMOVE: #include <bg2e/render/vulkan/Instance.hpp>

void PhysicalDevice::listSuitableDevices(
    VkInstance instance,              // was: const Instance&
    const Surface& surface,
    std::vector<std::shared_ptr<PhysicalDeviceProperties>>& result
) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);  // was: instance.handle()
    // ... rest of logic unchanged, just replace instance.handle() with instance
}

void PhysicalDevice::choose(VkInstance instance, const Surface& surface)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);  // was: instance.handle()
    // ... rest unchanged
}

void PhysicalDevice::choose(VkInstance instance)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);  // was: instance.handle()
    // ... rest unchanged
}
```

**Summary of changes in PhysicalDevice.cpp:**
- Remove `#include <bg2e/render/vulkan/Instance.hpp>`
- Replace all `instance.handle()` with `instance` (6 occurrences across `listSuitableDevices` and both `choose` overloads)

---

## 2.3 — Update `Device`

### Header: `lib/include/bg2e/render/vulkan/Device.hpp`

```cpp
class BG2E_API Device {
public:
    void create(VkInstance instance, const PhysicalDevice& physicalDevice, bool offscreen);
    // was: const Instance& instance
    // ... rest unchanged
};
```

### Source: `lib/src/bg2e/render/vulkan/Device.cpp`

The Device uses `instance.getRequiredLayers(requiredLayers)` to get validation layers. Since
this is a method on the Instance class, we need to inline this logic or change the approach.

**Strategy:** The Device only needs to know if validation layers are available and should be
enabled. Instead of calling `instance.getRequiredLayers()`, we inline the layer query logic:

```cpp
// REMOVE: #include <bg2e/render/vulkan/Instance.hpp>

void Device::create(VkInstance instance, const PhysicalDevice& physicalDevice, bool offscreen)
{
    // ... existing queue creation code unchanged ...

    // --- FEATURE CHAIN --- (unchanged)

    // --- DEVICE CREATE --- (unchanged until layers section)

    // Validation layers for device (replaces instance.getRequiredLayers() call)
    std::vector<const char*> requiredLayers;
    if (base::Log::isDebug())
    {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const auto& layer : availableLayers)
        {
            if (std::string(layer.layerName) == "VK_LAYER_KHRONOS_validation")
            {
                requiredLayers.push_back("VK_LAYER_KHRONOS_validation");
                break;
            }
        }
    }

    if (!requiredLayers.empty())
    {
        createInfo.enabledLayerCount = uint32_t(requiredLayers.size());
        createInfo.ppEnabledLayerNames = requiredLayers.data();
    }

    // ... rest unchanged ...
}
```

**Rationale:** The original code called `instance.getRequiredLayers(requiredLayers)` which
had the side effect of modifying a static member on Instance. By inlining the layer check,
Device becomes fully independent of Instance. The check is simple: enumerate instance layers,
look for `VK_LAYER_KHRONOS_validation`, and add it if found in debug mode.

---

## 2.4 — Update `Engine.cpp` Call Sites

**File:** `lib/src/bg2e/render/Engine.cpp`

Update all calls that pass `_instance` to dependent classes:

```cpp
void Engine::createSurface()
{
    _surface.create(_instance.handle(), _windowPtr);  // was: _surface.create(_instance, _windowPtr)
}

void Engine::createDevicesAndQueues()
{
    if (isOffscreen())
    {
        _physicalDevice.choose(_instance.handle());  // was: _physicalDevice.choose(_instance)
    }
    else
    {
        _physicalDevice.choose(_instance.handle(), _surface);  // was: _physicalDevice.choose(_instance, _surface)
    }

    _device.create(_instance.handle(), _physicalDevice, isOffscreen());
    // was: _device.create(_instance, _physicalDevice, isOffscreen())
    _command.init(this);
}
```

**Note:** Engine still uses `render::vulkan::Instance` as its member. That changes in Phase 3.

---

## 2.5 — Update `GPUSelectionDialog.cpp` Call Sites

**File:** `lib/src/bg2e/app/GPUSelectionDialog.cpp`

```cpp
void getAvailableDevices(std::vector<std::shared_ptr<render::vulkan::PhysicalDeviceProperties>>& result)
{
    // ... window creation unchanged ...

    render::vulkan::Instance instance;
    instance.create(dummyWindow);

    render::vulkan::Surface surface;
    surface.create(instance.handle(), dummyWindow);  // was: surface.create(instance, dummyWindow)

    render::vulkan::PhysicalDevice::listSuitableDevices(
        instance.handle(), surface, result  // was: instance, surface, result
    );
    surface.cleanup();
    instance.cleanup();

    // ... rest unchanged ...
}
```

---

## 2.6 — Verification

1. **Compile check:** Build the project. All changes are API-compatible — the only difference
   is `VkInstance` vs `const Instance&` which is a compatible call site change via
   `instance.handle()`.
2. **Runtime check:** Run the engine (any example or the model editor). Verify:
   - Windowed mode works (Surface creation, device selection, swapchain)
   - Validation layers still appear in debug builds
   - GPUSelectionDialog still works if applicable
3. **No behavior change:** The logic is identical, only the parameter type changed.

---

## Design Decisions

| Decision | Rationale |
|----------|-----------|
| Dependent classes take `VkInstance` directly | They never needed the Instance wrapper — only `instance.handle()`. Direct VkInstance is simpler and fully decouples them. |
| Device inlines layer query | Avoids Device depending on any Instance class. The layer query is ~10 lines and self-contained. |
| Forward declarations removed | No longer needed since the classes don't reference Instance at all. |
| Engine.cpp updated in same phase | Keeps everything consistent — dependent classes accept VkInstance, callers pass `.handle()`. |
