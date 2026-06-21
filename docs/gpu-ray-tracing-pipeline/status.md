# Plan Status

## Step 01 completed: Extend ShaderStage and BufferUsage enums
Date: 2026-06-21
Changes:
- lib/include/bg2e/gpu/Common.hpp: Added RayGeneration, Miss, ClosestHit to ShaderStage enum; added ShaderBindingTable to BufferUsage enum; updated validatePushConstantRanges() with RT stage validation; updated validateMetalBufferBindings() with RT stage Metal buffer index validation
- lib/src/bg2e/gpu/vk/PipelineLayout.cpp: Added VK_SHADER_STAGE_RAYGEN_BIT_KHR, VK_SHADER_STAGE_MISS_BIT_KHR, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR to shaderStageToVkFlags()

## Step 02 completed: Extend ShaderLib with RT Shader Loading
Date: 2026-06-21
Changes:
- lib/include/bg2e/gpu/ShaderLib.hpp: Added rayGeneration(), miss(), closestHit() public methods; added _loadOrNull() private method
- lib/src/bg2e/gpu/ShaderLib.cpp: Implemented _loadOrNull() with Metal null-safety for missing miss/closestHit files; implemented rayGeneration() via _load(), miss() and closestHit() via _loadOrNull()

## Step 03 completed: Create RayTracingPipeline Abstract Class
Date: 2026-06-21
Changes:
- lib/include/bg2e/gpu/RayTracingPipeline.hpp: New file with RayTracingPipelineDescription struct and RayTracingPipeline class inheriting DeviceResource
- lib/include/bg2e/gpu/Device.hpp: Added forward declaration, include for RayTracingPipeline, and createRayTracingPipeline() factory method
- lib/include/bg2e/gpu/all.hpp: Added include for RayTracingPipeline.hpp

## Step 04 completed: Add traceRays to CommandBuffer
Date: 2026-06-21
Changes:
- lib/include/bg2e/gpu/CommandBuffer.hpp: Added forward declaration for RayTracingPipeline; added bindPipeline(RayTracingPipeline*), bindResourceSet(RayTracingPipeline*, uint32_t, ResourceSet*), and traceRays(uint32_t, uint32_t, uint32_t) virtual methods with default throw implementations
- lib/include/bg2e/gpu/metal/CommandBuffer.hpp: Added method declarations and _boundRayTracingPipeline member
- lib/include/bg2e/gpu/vk/CommandBuffer.hpp: Added method declarations for bindPipeline, bindResourceSet, and traceRays
- lib/src/bg2e/gpu/metal/CommandBuffer.cpp: Implemented bindPipeline(RayTracingPipeline*) creating compute encoder; implemented bindResourceSet binding textures, buffers, samplers, and acceleration structures to compute encoder; implemented traceRays dispatching threadgroups; added RayGeneration/Miss/ClosestHit cases to pushConstants switch
- lib/src/bg2e/gpu/vk/CommandBuffer.cpp: Added throw implementations for bindPipeline(RayTracingPipeline*), bindResourceSet(RayTracingPipeline*, ...), and traceRays

## Step 05 completed: Vulkan RayTracingPipeline Backend
Date: 2026-06-21
Changes:
- lib/include/bg2e/gpu/vk/RayTracingPipeline.hpp: New file with Vulkan RayTracingPipeline class inheriting gpu::RayTracingPipeline, with SBT regions and pipeline handle accessors
- lib/src/bg2e/gpu/vk/RayTracingPipeline.cpp: Implemented Vulkan RT pipeline creation with shader stages (raygen, miss, closest hit), shader group management, automatic SBT creation with proper alignment, and debug naming
- lib/include/bg2e/gpu/vk/Device.hpp: Added createRayTracingPipeline() override declaration; added _physicalDevice member and physicalDeviceHandle() accessor
- lib/src/bg2e/gpu/vk/Device.cpp: Added include for RayTracingPipeline.hpp; implemented createRayTracingPipeline() factory method; stored physical device handle during create()
- lib/src/bg2e/gpu/vk/Buffer.cpp: Added toVkBufferUsage() helper function converting BufferUsage flags to VkBufferUsageFlags including ShaderBindingTable mapping

## Step 06 completed: Metal RayTracingPipeline backend
Date: 2026-06-21
Changes:
- lib/include/bg2e/gpu/metal/RayTracingPipeline.hpp: New file with Metal RayTracingPipeline class inheriting gpu::RayTracingPipeline, wraps MTLComputePipelineState from rgen shader module
- lib/src/bg2e/gpu/metal/RayTracingPipeline.cpp: Implemented Metal RT pipeline creation from rgen compute kernel; miss/closestHit silently ignored on Metal; includes Mac and non-Mac fallback implementations
- lib/include/bg2e/gpu/metal/Device.hpp: Added createRayTracingPipeline() override declaration
- lib/src/bg2e/gpu/metal/Device.cpp: Added include for RayTracingPipeline.hpp; implemented createRayTracingPipeline() factory method with Mac and non-Mac fallbacks

## Step 07 completed: Vulkan CommandBuffer RT integration
Date: 2026-06-21
Changes:
- lib/include/bg2e/gpu/vk/CommandBuffer.hpp: Added _boundRTPipeline member variable
- lib/src/bg2e/gpu/vk/CommandBuffer.cpp: Added include for RayTracingPipeline.hpp; implemented bindPipeline(RayTracingPipeline*) with vkCmdBindPipeline using VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR; implemented bindResourceSet(RayTracingPipeline*) with vkCmdBindDescriptorSets using RT bind point; implemented traceRays() calling cmdTraceRays with SBT regions from bound pipeline; reset _boundRTPipeline in end() method

## Step 08 completed: Metal CommandBuffer RT Integration
Date: 2026-06-21
Changes:
- lib/src/bg2e/gpu/metal/CommandBuffer.cpp: Added include for RayTracingPipeline.hpp; rewritten bindPipeline(RayTracingPipeline*) to auto-manage compute encoder with dynamic_cast validation and layout binding; rewritten bindResourceSet(RayTracingPipeline*) with Metal resource set type checking and rtScene residency; rewritten traceRays() to use constexpr 8x8 threadgroups matching Metal shader

## Step 09 completed: Validation Example — Cornell Box
Date: 2026-06-21
Changes:
- examples/gpu/13_ray_tracing_pipeline/CMakeLists.txt: New CMake build file with shader compilation and resource bundling
- examples/gpu/13_ray_tracing_pipeline/shaders/ray_tracing_pipeline.rgen.glsl: Vulkan ray generation shader with camera, TLAS trace, hit payload, and progressive accumulation
- examples/gpu/13_ray_tracing_pipeline/shaders/ray_tracing_pipeline.rmiss.glsl: Vulkan miss shader returning dark sky color
- examples/gpu/13_ray_tracing_pipeline/shaders/ray_tracing_pipeline.rchit.glsl: Vulkan closest hit shader with Lambert lighting and per-primitive HitData from SSBO
- examples/gpu/13_ray_tracing_pipeline/shaders/ray_tracing_pipeline.rgen.metal: Metal compute kernel with intersector ray query, miss/hit logic merged, and progressive accumulation
- examples/gpu/13_ray_tracing_pipeline/src/main.cpp: Full example with Cornell box scene (floor, ceiling, walls, light, cube, sphere), BLAS/TLAS build, SSBO hit data, resource sets, RT pipeline, render loop with resize handling

## Step 10 completed: Documentation
Date: 2026-06-21
Changes:
- doc/api/gpu/RayTracingPipeline.md: New file with full API documentation covering RayTracingPipelineDescription, RayTracingPipeline class, Device::createRayTracingPipeline(), CommandBuffer RT methods (bindPipeline, bindResourceSet, traceRays), usage example, and backend differences
- doc/api/gpu/Common.md: Updated ShaderStage enum with RayGeneration, Miss, ClosestHit values; added comment about Metal null-safety for miss/closestHit; updated BufferUsage enum with ShaderBindingTable flag
- doc/api/gpu/ShaderLibraries.md: Added rayGeneration(), miss(), closestHit() method declarations to ShaderLib class; added new section documenting RT shader loading with Metal null-safety notes and file resolution table
- doc/api/gpu/CommandBuffer.md: Added bindPipeline(RayTracingPipeline*), bindResourceSet(RayTracingPipeline*, uint32_t, ResourceSet*), and traceRays(uint32_t, uint32_t, uint32_t) to class declaration and documentation sections
- doc/api/gpu/Device.md: Added createRayTracingPipeline() to abstract class declaration and documentation; added override declarations to vk::Device and metal::Device class listings
- doc/api/gpu/reference.md: Added RayTracingPipelineDescription struct entry, RayTracingPipeline class entry, and RayTracingPipeline abstract node to class hierarchy
- doc/api/gpu/quick_start.md: Added RayTracingPipeline to class catalog and ShaderLib methods; updated ShaderStage and BufferUsage enum types with RT values; added RayTracingPipelineDescription struct entry; added Recipe 10 (Ray Tracing Pipeline) with complete shader loading, pipeline layout, BLAS/TLAS building, pipeline creation, and traceRays dispatch code
- doc/api/gpu/index.md: Added RayTracingPipeline to object creation flow and class hierarchy; updated ray tracing section with new RayTracingPipeline subsection including typical data flow code; added vk::RayTracingPipeline and metal::RayTracingPipeline to backend-specific accessors table
