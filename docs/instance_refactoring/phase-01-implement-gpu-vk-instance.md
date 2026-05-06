# Phase 1: Implement `bg2e::gpu::vk::Instance`

## Objective

Migrate the full Vulkan instance implementation from `bg2e::render::vulkan::Instance` to
`bg2e::gpu::vk::Instance`, completing the existing stub with all production logic. Also update
the abstract interface `bg2e::gpu::Instance` to support headless (offscreen) creation.

**After this phase:** The new `gpu::vk::Instance` is fully functional. The old
`render::vulkan::Instance` still exists and is still used by Engine and other code. Both
classes can coexist — no existing code breaks.

---

## Files to Modify

| File | Action |
|------|--------|
| `lib/include/bg2e/gpu/Instance.hpp` | Modify — add `create()` headless method |
| `lib/include/bg2e/gpu/vk/Info.hpp` | New — Vulkan struct factory (migrated from render::vulkan) |
| `lib/src/bg2e/gpu/vk/Info.cpp` | New — Info implementation |
| `lib/include/bg2e/gpu/vk/Instance.hpp` | Modify — add full member variables and internal methods |
| `lib/src/bg2e/gpu/vk/Instance.cpp` | Modify — implement all methods with production logic |

---

## 1.1 — Update Abstract Interface: `gpu::Instance`

**File:** `lib/include/bg2e/gpu/Instance.hpp`

Add the headless `create()` method to the abstract interface:

```cpp
class BG2E_API Instance {
public:
    virtual ~Instance() = default;

    virtual void enableDebugMode(bool value) = 0;
    [[nodiscard]] virtual bool debugModeEnabled() const = 0;
    virtual void setApplicationName(const std::string& name) = 0;
    [[nodiscard]] virtual const std::string& applicationName() const = 0;

    // Windowed application
    virtual void create(SDL_Window* window) = 0;
    // Headless / offscreen application
    virtual void create() = 0;

    virtual void cleanup() = 0;
};
```

**Rationale:** The existing code in `Engine::createInstance()` branches between
`_instance.create()` (offscreen) and `_instance.create(_windowPtr)` (windowed). The abstract
interface must support both paths.

---

## 1.2 — Update Vulkan Implementation Header: `gpu::vk::Instance`

**File:** `lib/include/bg2e/gpu/vk/Instance.hpp`

Replace the current stub with a full implementation. The new header should include:

```cpp
#pragma once

#include <bg2e/gpu/Instance.hpp>
#include <bg2e/gpu/vk/common.hpp>

#include <vector>
#include <string>

namespace bg2e {
namespace gpu {
namespace vk {

class Instance : public gpu::Instance {
public:
    Instance();

    // gpu::Instance interface
    void setApplicationName(const std::string& name) override;
    const std::string& applicationName() const override;
    void enableDebugMode(bool value) override;
    bool debugModeEnabled() const override;

    void create(SDL_Window* window) override;
    void create() override;
    void cleanup() override;

    // Vulkan-specific accessor (not part of abstract interface)
    VkInstance vkInstanceHnd() const { return _instance; }

    // Layer and extension queries — used by Device during creation
    bool getRequiredLayers(std::vector<const char*>& requiredLayers) const;
    bool getRequiredExtensions(SDL_Window* window, std::vector<const char*>& requiredExtensions) const;
    bool getRequiredExtensions(std::vector<const char*>& requiredExtensions) const;

private:
    VkInstance _instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;

    bool _debugMode = false;
    static bool s_debugLayerAvailable;
    std::string _applicationName = "bg2 engine Vulkan Application";

    std::vector<std::string> _availableExtensions;
    std::vector<std::string> _availableLayers;

    VkResult createDebugMessenger();
    void destroyDebugMessenger();
};

}
}
}
```

**Key differences from `render::vulkan::Instance`:**
- `enableValidationLayers(bool)` → `enableDebugMode(bool)` (matches abstract interface naming)
- `isValidationLayersEnabled()` → `debugModeEnabled()` (matches abstract interface naming)
- `handle()` → `vkInstanceHnd()` (Vulkan-specific, not in abstract interface)
- Constructor enumerates available layers and extensions (same as original)
- `getRequiredLayers()` and `getRequiredExtensions()` remain as public methods for Device usage

---

## 1.3 — Create Vulkan Struct Factory: `gpu::vk::Info`

**File:** `lib/include/bg2e/gpu/vk/Info.hpp`
**File:** `lib/src/bg2e/gpu/vk/Info.cpp`

Migrate the full `render::vulkan::Info` class to the new `gpu::vk` namespace. This provides
all Vulkan struct factory methods needed by Instance and future classes in the `gpu` module,
eliminating any dependency on `render::vulkan`.

```cpp
class BG2E_API Info {
public:
    static VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo(...);
    static VkCommandPoolCreateInfo commandPoolCreateInfo(...);
    // ... (all methods copied from render::vulkan::Info)
};
```

**Methods migrated:** `debugMessengerCreateInfo`, `commandPoolCreateInfo`, `commandBufferAllocateInfo`,
`fenceCreateInfo`, `semaphoreCreateInfo`, `commandBufferBeginInfo`, `semaphoreSubmitInfo`,
`commandBufferSubmitInfo`, `submitInfo` (2 overloads), `presentInfo`, `imageCreateInfo`,
`imageViewCreateInfo`, `attachmentInfo`, `depthAttachmentInfo`, `renderingInfo`, `pipelineLayoutInfo`.

### Dependency resolution

`gpu::vk::Instance` now uses `#include <bg2e/gpu/vk/Info.hpp>` instead of
`#include <bg2e/render/vulkan/Info.hpp>`. The `gpu::vk` module is self-contained
with respect to Vulkan struct creation — no external rendering engine dependencies.

---

## 1.4 — Implement Vulkan Logic: `gpu::vk::Instance.cpp`

**File:** `lib/src/bg2e/gpu/vk/Instance.cpp`

Move all production logic from `render::vulkan::Instance.cpp` to this file. The implementation
is essentially identical with the following changes:

### Static member initialization
```cpp
bool Instance::s_debugLayerAvailable = false;
```

### Debug callback (identical to original)
The `bg2e_mainDebugCallback` static function is copied verbatim.

### Constructor (identical to original)
Enumerates available instance layers and extensions, logging them.

### `create(SDL_Window*)` (identical logic to original)
- Gets required layers via `getRequiredLayers()`
- Gets required extensions via `getRequiredExtensions(window, ...)`
- Builds `VkApplicationInfo` with Vulkan 1.3
- Chains debug messenger create info via `Info::debugMessengerCreateInfo()`
- Calls `vkCreateInstance()`
- Creates debug messenger if debug mode is enabled

### `create()` — headless (identical logic to original headless path)
Same as above but without SDL window extensions.

### `cleanup()` (identical to original)
Destroys debug messenger, then calls `vkDestroyInstance()`.

### `getRequiredLayers()` (identical to original)
Checks for `VK_LAYER_KHRONOS_validation`, adds it if available and debug is enabled.

### `getRequiredExtensions(SDL_Window*, ...)` (fix bug)
Same as original **but fix the duplicate extension push bug**:
```cpp
// BEFORE (buggy - line 272 pushes DEBUG_UTILS twice):
requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

// AFTER (fixed):
requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
```

### `getRequiredExtensions()` — headless (identical to original)

### `createDebugMessenger()` / `destroyDebugMessenger()` (identical to original)

### Dependencies required
- `#include <bg2e/gpu/vk/Info.hpp>` — local Vulkan struct factory (self-contained, no external deps)
- `#include <bg2e/base/Log.hpp>` — for `bg2e_log_*` macros and `base::Log::isDebug()`
- `#include <SDL2/SDL_vulkan.h>` — for `SDL_Vulkan_GetInstanceExtensions()`

The `gpu::vk` module is fully self-contained — Instance depends only on its own local Info class,
the logging subsystem, and SDL2. No dependency on `render::vulkan` or any other engine module.

---

## 1.5 — Verification

After completing this phase:

1. **Compile check:** Build the project. The new `gpu::vk::Instance` compiles alongside the
   existing `render::vulkan::Instance` with no conflicts. The new `gpu::vk::Info` class is
   compiled and linked into the engine.
2. **No runtime changes:** Engine still uses `render::vulkan::Instance`. The new class is not
   called from production code yet.
3. **Manual smoke test:** Optionally, write a temporary test that creates a `gpu::vk::Instance`,
   calls `create()`, and verifies the VkInstance handle is valid. Remove the test afterwards.

---

## Design Decisions

| Decision | Rationale |
|----------|-----------|
| `getRequiredLayers/Extensions` stay public on `vk::Instance` | Device needs them during creation. These are Vulkan-specific, so they belong in the concrete class. |
| `s_debugLayerAvailable` remains static | Matches original behavior. Can be refactored to instance member in a future phase. |
| `render::vulkan::Info` migrated to `gpu::vk::Info` | Complete migration — `gpu::vk` module is fully self-contained with zero external rendering dependencies. |
| `enableValidationLayers` renamed to `enableDebugMode` | Aligns with abstract interface naming and future multi-backend semantics. |
| Bug fix included | The duplicate `VK_EXT_DEBUG_UTILS_EXTENSION_NAME` push is fixed during migration. |
