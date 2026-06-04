# GPU API Reference

Class, struct, and enum reference for the `bg2e::gpu` namespace and its
backend-specific sub-namespaces (`bg2e::gpu::vk`, `bg2e::gpu::metal`).

---

## Enums

| Enum | Header | Description |
|------|--------|-------------|
| [BackendType](BackendType.md) | `gpu/Common.hpp` | Selects Vulkan or Metal backend. |
| [WindowType](WindowType.md) | `gpu/Backend.hpp` | SDL window flag type for a backend. |
| [PresentationMode](PresentationMode.md) | `gpu/Instance.hpp` | Windowed vs offscreen creation mode. |

## Structs

| Struct | Header | Description |
|--------|--------|-------------|
| [PhysicalDeviceProperties](PhysicalDeviceProperties.md) | `gpu/PhysicalDevice.hpp` | GPU properties, scoring, and ray tracing caps. Includes `DeviceType` enum and `RayTracingCapabilities` struct. |

## Classes

| Class | Header | Description |
|-------|--------|-------------|
| [Factory](Factory.md) | `gpu/Factory.hpp` | Entry point; creates the backend singleton. |
| [Backend](Backend.md) | `gpu/Backend.hpp` | Abstract factory for all GPU subsystem objects. |
| [Instance](Instance.md) | `gpu/Instance.hpp` | GPU API instance (Vulkan instance / Metal device system). |
| [Surface](Surface.md) | `gpu/Surface.hpp` | Abstract base for rendering surfaces. |
| [WindowSurface](WindowSurface.md) | `gpu/WindowSurface.hpp` | Surface backed by an OS window. |
| [OffscreenSurface](OffscreenSurface.md) | `gpu/OffscreenSurface.hpp` | Surface for headless rendering. |
| [PhysicalDevice](PhysicalDevice.md) | `gpu/PhysicalDevice.hpp` | Physical GPU device selection and properties. |
| [Device](Device.md) | `gpu/Device.hpp` | Logical device and command queue access. |
| [Queue](Queue.md) | `gpu/Queue.hpp` | Abstract command queue. |

## Vulkan utilities

| Item | Header | Description |
|------|--------|-------------|
| [vk::Info](Info.md) | `gpu/vk/Info.hpp` | Static factory for Vulkan struct initialization. |
| [VK_ASSERT](Info.md#vk_assert) | `gpu/vk/common.hpp` | Error-checking macro for `VkResult`. |

## Metal utilities

| Item | Header | Description |
|------|--------|-------------|
| [Metal handle types](MetalHandleTypes.md) | `gpu/metal/common.hpp` | Type aliases for `MTL::Device*`, `MTL::CommandQueue*`, `CA::MetalLayer*`. |

---

## Class hierarchy

```
gpu::Factory                  (static, no inheritance)
gpu::Backend                  (abstract)
  +-- gpu::vk::Backend
  +-- gpu::metal::Backend
gpu::Instance                 (abstract)
  +-- gpu::vk::Instance
  +-- gpu::metal::Instance
gpu::Surface                  (abstract)
  +-- gpu::WindowSurface      (abstract)
  |     +-- gpu::vk::WindowSurface
  |     +-- gpu::metal::WindowSurface
  +-- gpu::OffscreenSurface   (concrete base)
        +-- gpu::vk::OffscreenSurface
        +-- gpu::metal::OffscreenSurface
gpu::PhysicalDevice           (abstract)
  +-- gpu::vk::PhysicalDevice
  +-- gpu::metal::PhysicalDevice
gpu::Device                   (abstract)
  +-- gpu::vk::Device
  +-- gpu::metal::Device
gpu::Queue                    (abstract)
  +-- gpu::vk::Queue
  +-- gpu::metal::Queue
```
