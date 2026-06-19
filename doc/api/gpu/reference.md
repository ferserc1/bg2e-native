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
| [ShaderStage](Common.md#shaderstage) | `gpu/Common.hpp` | Pipeline stage: Vertex, Fragment, Compute. |
| [ImageLayout](Common.md#imagelayout) | `gpu/Common.hpp` | Image layout states for transitions. |
| [PixelFormat](Common.md#pixelformat) | `gpu/Common.hpp` | Pixel format for images and attachments. |
| [ResourceType](Common.md#resourcetype) | `gpu/Common.hpp` | Kind of resource at a descriptor binding: UBO, SSBO, image, sampler, acceleration structure. |
| [PrimitiveTopology](GraphicsPipeline.md#primitivetopology) | `gpu/GraphicsPipeline.hpp` | Primitive assembly topology. |

## Structs

| Struct | Header | Description |
|--------|--------|-------------|
| [PhysicalDeviceProperties](PhysicalDeviceProperties.md) | `gpu/PhysicalDevice.hpp` | GPU properties, scoring, and ray tracing caps. Includes `DeviceType` enum and `RayTracingCapabilities` struct. |
| [Color](Common.md#color) | `gpu/Common.hpp` | RGBA float color value. |
| [Size2D](Common.md#size2d) | `gpu/Common.hpp` | 2D dimensions (width, height). |
| [Size3D](Common.md#size3d) | `gpu/Common.hpp` | 3D dimensions (width, height, depth). |
| [ShaderModuleDescription](ShaderModule.md#shadermoduledescription) | `gpu/Common.hpp` | Shader file path, entry point, and stage. |
| [PushConstantRange](Common.md#pushconstantrange) | `gpu/Common.hpp` | Push constant offset, size, and stage. |
| [ShaderBinding](Common.md#shaderbinding) | `gpu/Common.hpp` | Backend-specific binding indices: Vulkan descriptor binding + Metal argument index. |
| [ResourceBinding](Common.md#resourcebinding) | `gpu/Common.hpp` | Single descriptor binding: set, ShaderBinding (Vulkan + Metal indices), type, stage, and array count. |
| [PipelineLayoutDescription](Common.md#pipelinelayoutdescription) | `gpu/Common.hpp` | Push constants + resource bindings for a pipeline layout. |
| [GraphicsPipelineDescription](GraphicsPipeline.md#graphicspipelinedescription) | `gpu/GraphicsPipeline.hpp` | Shaders, layout, topology, and formats. |
| [ComputePipelineDescription](ComputePipeline.md#computepipelinedescription) | `gpu/ComputePipeline.hpp` | Compute shader and layout. |
| [RayTracingMeshDescription](RayTracingMesh.md#raytracingmeshdescription) | `gpu/RayTracingMesh.hpp` | Shared vertex/index buffers + submesh range for a bottom-level acceleration structure. |
| [RayTracingInstance](RayTracingScene.md#raytracinginstance) | `gpu/RayTracingScene.hpp` | One instance of a RayTracingMesh in a scene: mesh, transform, id, mask. |

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
| [Device](Device.md) | `gpu/Device.hpp` | Logical device, queue access, and resource factories. |
| [Queue](Queue.md) | `gpu/Queue.hpp` | Abstract command queue; creates and submits command buffers. |
| [ShaderModule](ShaderModule.md) | `gpu/ShaderModule.hpp` | Compiled shader module (SPIR-V or metallib). |
| [ShaderLib](ShaderLibraries.md) | `gpu/ShaderLib.hpp` | Backend-agnostic shader library loader; wraps a directory of compiled shaders. |
| [PipelineLayout](PipelineLayout.md) | `gpu/PipelineLayout.hpp` | Push constant and descriptor set layout. |
| [GraphicsPipeline](GraphicsPipeline.md) | `gpu/GraphicsPipeline.hpp` | Graphics pipeline state object. |
| [ComputePipeline](ComputePipeline.md) | `gpu/ComputePipeline.hpp` | Compute pipeline state object. |
| [CommandBuffer](CommandBuffer.md) | `gpu/CommandBuffer.hpp` | Records GPU commands for queue submission. |
| [SurfaceFrame](SurfaceFrame.md) | `gpu/SurfaceFrame.hpp` | Frame from a surface swapchain. |
| [Image](Image.md) | `gpu/Image.hpp` | GPU image (texture or render target). |
| [ResourceSet](ResourceSet.md) | `gpu/ResourceSet.hpp` | Groups resources (UBOs, SSBOs, images, samplers, acceleration structures) bound to one descriptor set. |
| [RayTracingMesh](RayTracingMesh.md) | `gpu/RayTracingMesh.hpp` | Bottom-level acceleration structure (BLAS) for one submesh, from existing GPU buffers. |
| [RayTracingScene](RayTracingScene.md) | `gpu/RayTracingScene.hpp` | Top-level acceleration structure (TLAS) holding instances of RayTracingMesh. |
| [`MeshGeneric<T>`](Mesh.md) | `gpu/Mesh.hpp` | Template pairing CPU mesh data with GPU vertex/index buffers. |
| [`CleanupManager`](CleanupManager.md) | `gpu/CleanupManager.hpp` | Ordered and deferred cleanup for device resources. |
| [`FrameResourceRing<T>`](FrameResourceRing.md) | `gpu/FrameResourceRing.hpp` | Template ring of per-frame device resources. |

## Vulkan utilities

| Item | Header | Description |
|------|--------|-------------|
| [vk::Info](Info.md) | `gpu/vk/Info.hpp` | Static factory for Vulkan struct initialization. |
| [VK_ASSERT](Info.md#vk_assert) | `gpu/vk/common.hpp` | Error-checking macro for `VkResult`. |

## Metal utilities

| Item | Header | Description |
|------|--------|-------------|
| [Metal handle types](MetalHandleTypes.md) | `gpu/metal/common.hpp` | Type aliases for `MTL::Device*`, `MTL::CommandQueue*`, `CA::MetalLayer*. |

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
gpu::ShaderModule             (abstract)
  +-- gpu::vk::ShaderModule
  +-- gpu::metal::ShaderModule
gpu::PipelineLayout           (abstract)
  +-- gpu::vk::PipelineLayout
  +-- gpu::metal::PipelineLayout
gpu::GraphicsPipeline         (abstract)
  +-- gpu::vk::GraphicsPipeline
  +-- gpu::metal::GraphicsPipeline
gpu::ComputePipeline          (abstract)
  +-- gpu::vk::ComputePipeline
  +-- gpu::metal::ComputePipeline
gpu::CommandBuffer            (abstract)
  +-- gpu::vk::CommandBuffer
  +-- gpu::metal::CommandBuffer
gpu::SurfaceFrame             (abstract)
  +-- gpu::vk::SurfaceFrame
  +-- gpu::metal::SurfaceFrame
gpu::Image                    (abstract)
  +-- gpu::vk::Image
  +-- gpu::metal::Image
gpu::ResourceSet              (abstract)
  +-- gpu::vk::ResourceSet
  +-- gpu::metal::ResourceSet
gpu::RayTracingMesh           (abstract)
  +-- gpu::vk::RayTracingMesh
  +-- gpu::metal::RayTracingMesh
gpu::RayTracingScene          (abstract)
  +-- gpu::vk::RayTracingScene
  +-- gpu::metal::RayTracingScene
gpu::CleanupManager           (concrete, not polymorphic)
gpu::FrameResourceRing<T>     (template, not polymorphic)
```
