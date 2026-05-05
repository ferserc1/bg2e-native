# Instance Refactoring Plan — Summary

## Overview

This plan refactors the Vulkan Instance management from `bg2e::render::vulkan::Instance` to
the new multi-backend abstraction layer at `bg2e::gpu::Instance` (abstract interface) and
`bg2e::gpu::vk::Instance` (Vulkan implementation).

This is the first step toward a multi-backend GPU abstraction system. The `gpu` module already
exists as scaffolding with stub implementations — this plan fills in the Instance implementation
and migrates all callers.

---

## Architecture

```
bg2e::gpu::Instance           (abstract interface — platform-agnostic)
    |
    +-- bg2e::gpu::vk::Instance   (Vulkan implementation — owns VkInstance)

bg2e::render::Engine          (owns gpu::vk::Instance via unique_ptr)
    |                           exposes gpu::Instance* publicly
    |
    +-- vulkan::Surface         (takes VkInstance directly)
    +-- vulkan::PhysicalDevice  (takes VkInstance directly)
    +-- vulkan::Device          (takes VkInstance directly, inlines layer query)
```

---

## Phases

| Phase | Description | Files Modified | Risk |
|-------|-------------|----------------|------|
| **1** | Implement `gpu::vk::Instance` | 3 files | Low — new code, no existing code changed |
| **2** | Update dependent classes (Surface, PhysicalDevice, Device) | 8 files | Medium — API signature changes |
| **3** | Update Engine to use `gpu::Instance` | 2 files | Medium — ownership change |
| **4** | Update GPUSelectionDialog | 1 file | Low — isolated change |
| **5** | Remove old `render::vulkan::Instance` | 3 files | Low — deletion only, all callers migrated |

Each phase results in a compilable, functional engine.

---

## Phase 1: Implement `gpu::vk::Instance`

**Goal:** Fill in the existing stub with all production logic from `render::vulkan::Instance`.

- Add `create()` (headless) to abstract `gpu::Instance` interface
- Move all Vulkan instance creation logic to `gpu::vk::Instance`
- Fix the duplicate `VK_EXT_DEBUG_UTILS_EXTENSION_NAME` bug (line 272 of original)
- Rename `enableValidationLayers` → `enableDebugMode` to match abstract interface

**Detail:** [phase-01-implement-gpu-vk-instance.md](phase-01-implement-gpu-vk-instance.md)

---

## Phase 2: Update Dependent Classes

**Goal:** Decouple Surface, PhysicalDevice, and Device from the Instance class.

- `Surface::create()` → takes `VkInstance` instead of `const Instance&`
- `PhysicalDevice::choose()` / `listSuitableDevices()` → takes `VkInstance` instead of
  `const Instance&`
- `Device::create()` → takes `VkInstance` instead of `const Instance&`, inlines layer query
- Update Engine.cpp and GPUSelectionDialog.cpp call sites to pass `.handle()`

**Detail:** [phase-02-update-dependent-classes.md](phase-02-update-dependent-classes.md)

---

## Phase 3: Update Engine

**Goal:** Replace `render::vulkan::Instance` member with `gpu::vk::Instance` in Engine.

- Engine holds `std::unique_ptr<gpu::vk::Instance>` internally
- Public API exposes `gpu::Instance*` (abstract)
- Private `vkInstance()` helper provides `VkInstance` for internal Vulkan calls
- `createInstance()` creates `gpu::vk::Instance` via `make_unique`

**Detail:** [phase-03-update-engine.md](phase-03-update-engine.md)

---

## Phase 4: Update GPUSelectionDialog

**Goal:** Replace the last caller of `render::vulkan::Instance`.

- `render::vulkan::Instance` → `gpu::vk::Instance`
- Pass `instance.vkInstanceHnd()` to Surface and PhysicalDevice

**Detail:** [phase-04-update-gpu-selection-dialog.md](phase-04-update-gpu-selection-dialog.md)

---

## Phase 5: Remove Old Instance

**Goal:** Clean up the unused old class.

- Delete `lib/include/bg2e/render/vulkan/Instance.hpp`
- Delete `lib/src/bg2e/render/vulkan/Instance.cpp`
- Remove from `render/vulkan/all.hpp`

**Detail:** [phase-05-remove-old-instance.md](phase-05-remove-old-instance.md)

---

## Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| Abstract `gpu::Instance` has no `handle()` method | Platform-agnostic — `VkInstance` is Vulkan-specific. Only `gpu::vk::Instance` exposes `vkInstanceHnd()`. |
| Engine exposes `gpu::Instance*` publicly | Follows "Abstract only" strategy. External code works with the platform-agnostic interface. |
| Dependent classes take `VkInstance` directly | They never needed the wrapper — only the raw handle. This fully decouples them. |
| Device inlines layer query | Avoids Device depending on any Instance class. ~10 lines of self-contained logic. |
| Bug fix included in Phase 1 | The duplicate `VK_EXT_DEBUG_UTILS_EXTENSION_NAME` push is fixed during migration. |
| `enableValidationLayers` → `enableDebugMode` | Aligns with the abstract interface naming and future multi-backend semantics. |

---

## Files Changed (Total)

| Category | Files |
|----------|-------|
| **New/Modified** | `gpu/Instance.hpp`, `gpu/vk/Instance.hpp`, `gpu/vk/Instance.cpp` |
| **Modified** | `render/vulkan/Surface.hpp`, `render/vulkan/Surface.cpp` |
| **Modified** | `render/vulkan/PhysicalDevice.hpp`, `render/vulkan/PhysicalDevice.cpp` |
| **Modified** | `render/vulkan/Device.hpp`, `render/vulkan/Device.cpp` |
| **Modified** | `render/Engine.hpp`, `render/Engine.cpp` |
| **Modified** | `app/GPUSelectionDialog.cpp` |
| **Modified** | `render/vulkan/all.hpp` |
| **Deleted** | `render/vulkan/Instance.hpp`, `render/vulkan/Instance.cpp` |
| **Total** | 15 files |

---

## Dependencies and Risks

- **`render::vulkan::Info` dependency:** `gpu::vk::Instance` uses `Info::debugMessengerCreateInfo()`.
  This cross-namespace dependency is acceptable for now. `Info` can be migrated to `gpu::vk`
  in a future refactoring phase.
- **Static `s_debugLayerAvailable`:** The original code uses a mutable static member. This is
  preserved as-is to avoid behavior changes. Can be refactored to an instance member later.
- **`base::Log::isDebug()` dependency:** Both old and new Instance use this for debug mode
  detection. No change needed.
