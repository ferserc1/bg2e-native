# Tutorial 09: Ray Tracing Pipeline — Cornell Box Path Tracer

This tutorial adapts [GPU Ray Tracing Pipeline](../gpu/13_ray_tracing_pipeline.md)
to `bg2e::draw`. A path tracer renders a Cornell box with glossy objects,
progressively averaging noisy samples in an HDR image. A fullscreen graphics
pass displays the tone-mapped result in draw's retained color target before UI
composition.

**Source:** [examples/draw/09_ray_tracing_pipeline/src/main.cpp](../../../examples/draw/09_ray_tracing_pipeline/src/main.cpp)

## What you will learn

- How to create a ray tracing pipeline with ray generation, miss, and closest-hit shaders
- How to share immutable BLAS/TLAS and material data across frames in flight
- How to bind output/accumulation images alongside a per-slot camera UBO
- How to progressively average Monte Carlo samples and restart accumulation
- How to present a storage image through draw's color-only retained target
- How to close Metal's ray tracing compute scope before graphics rendering
- How to preserve background, pause, and refresh controls with a path tracer

## Prerequisites

- Completed [07_ray_query_shadows](07_ray_query_shadows.md)
- Familiarity with [03_uniform_buffers](03_uniform_buffers.md) and
  [04_render_to_texture](04_render_to_texture.md)
- A configured bg2e-native build environment with a valid Vulkan SDK
- A selected GPU/backend passing `PhysicalDeviceProperties::rayTracingSupported()`
- Compiled and packaged GLSL shaders and, on macOS, Metal shaders
- The Metal compiler toolchain on macOS

The device check preserves the source's full ray tracing capability predicate.
Unsupported devices fail with a descriptive startup error before scene shader
loading. On macOS, the Vulkan failure suggests Metal on a compatible Mac.
There is no fallback renderer. All geometry and materials are procedural, with
no external scene assets. The production `bg2e::render` API remains separate.

## Understanding the example

The scene preserves the source GPU example's eight pieces: five box walls,
a ceiling light rectangle, a mirror-like cube, and a mirror-like sphere.
The walls have roughness 0.5; the two objects have roughness 0.0. The ceiling
light has emission (18, 18, 18) and black albedo so paths terminate on it.

A 40-degree camera looks from (0, 2.7, 15) toward (0, 2.7, 0). Every active
scene frame traces one jittered path per pixel, with up to eight iterative
bounces, and incorporates it into a running mean. Noise should decrease as
more samples accumulate, although convergence depends on the scene and sampling.

The ray tracing API maps to different backend mechanisms:

| Backend | Implementation |
|---------|----------------|
| Vulkan | Native RT pipeline, backend-managed shader binding table, ray generation/miss/closest-hit stages |
| Metal | Compute pipeline running a ray generation kernel with hardware triangle intersectors and inline hit handling |

Draw's retained target supports color attachment and transfer-source usage;
it cannot receive storage writes or be the destination of the source example's
image copy. The delegate therefore owns an RGBA8 storage/sampled output and
an RGBA32F storage accumulation image, then samples output into retained color
using a fullscreen triangle. No depth attachment is needed for that pass.

The UI retains Background, Pause scene, Refresh scene, and counters, and adds
Restart accumulation and a sample count. The black default matches the source
background. Background changes affect primary camera rays that miss geometry;
escaped secondary paths still contribute no environment light.

## Step-by-step code explanation

### 1. Reuse the managed application lifecycle and capability check

The Application/MainLoop entry point follows preceding examples. The default
backend is Metal on macOS and Vulkan elsewhere. `--backend=vulkan` and
`--backend=metal` override that selection.

`initScene()` checks `_engine->physicalDevice()->properties()->rayTracingSupported()`
before creating scene resources, and stores the selected device name in the
shared controller. Draw owns device/surface setup, frame acquisition, command
submission, presentation, and global teardown.

### 2. Load RT and fullscreen shader modules

ShaderLib loads `ray_tracing_pipeline` through `rayGeneration()`, `miss()`, and
`closestHit()`, then loads the `output` vertex/fragment pair. Vulkan requires
all three RT modules. On Metal, the missing miss/closest-hit metallibs return
null because their logic is implemented inside the ray generation kernel.

The shader resource path is `shaders/draw_ray_tracing_pipeline`.
The RT layout preserves the source bindings:

| Resource | Vulkan set/binding | Metal index | Declared stage |
|----------|--------------------|-------------|----------------|
| Output storage image | 0 / 0 | Texture 0 | RayGeneration |
| Camera UBO | 0 / 1 | Buffer 1 | RayGeneration |
| HDR accumulation image | 0 / 2 | Texture 1 | RayGeneration |
| RayTracingScene | 1 / 0 | Buffer 2 | RayGeneration |
| Instance data SSBO | 2 / 0 | Buffer 3 | ClosestHit |

Metal binds the instance data buffer into its ray generation compute kernel.
The fullscreen layout is separate: set 0 contains sampled output at Vulkan
binding 0 / Metal texture 0 and a sampler at Vulkan binding 1 / Metal sampler 1.

### 3. Build Cornell box meshes and instance metadata

`ScenePiece` retains each `gpu::MeshPN`, its world transform, one material/shape
record, and its per-submesh `RayTracingMesh` objects. The procedural geometry
and transforms are unchanged from the GPU source.

Each metadata record contains four vec4 values:

```cpp
struct InstanceData {
    glm::vec4 albedo;         // rgb albedo, a = roughness
    glm::vec4 emission;       // rgb emission, a = shapeType
    glm::vec4 normalOrCenter; // plane normal or sphere/cube center; w = radius
    glm::vec4 extents;        // cube half-extents
};
```

Shape types are plane (0), sphere (1), and cube (2). Analytic normals are derived
from these records rather than additional vertex buffers bound to the shaders.
Plane normals incorporate their world transforms. Sphere normals use center
and hit position; the axis-aligned cube uses its center and half-extents.

### 4. Build immutable BLAS/TLAS and upload the SSBO

One immediate submission builds every submesh BLAS. The application then
creates a RayTracingScene and adds instances in the same loop that appends
metadata:

```cpp
uint32_t instanceId = 0;
for (auto& piece : _pieces)
{
    for (auto& rtMesh : piece->rtMeshes)
    {
        rayTracingScene->addInstance(rtMesh.get(), piece->model, instanceId++, 0xFF);
        allInstanceData.push_back(piece->instance);
    }
}
```

This ordering makes Vulkan's instance custom index and Metal's instance ID
select the matching metadata record. A second immediate submission builds the
TLAS before the scene set binds it. A storage buffer uploads the metadata.

The Cornell box is static, so BLAS, TLAS, and metadata remain shared and
immutable throughout active frames and window resizes. Unlike tutorial 07,
there is no demonstration rebuild on every frame.

### 5. Create the RT pipeline

The delegate creates the scene/data resource sets and a persistent pipeline:

```cpp
gpu::RayTracingPipelineDescription rt{};
rt.raygenShader = rgen.get();
rt.missShader = rmiss.get();
rt.closestHitShader = rchit.get();
rt.layout = _rtLayout.get();
rt.maxRecursionDepth = 1;
rt.debugName = "Cornell box RT pipeline";
_rtPipeline = device->createRayTracingPipeline(rt);
_cleanup->push(_rtPipeline);
```

Recursion depth 1 is sufficient: the ray generation shader iteratively launches
up to eight rays, rather than recursively tracing from closest-hit shaders.
The Vulkan backend manages the shader binding table as part of the pipeline.

### 6. Allocate images and camera/output rings on resize

Draw waits for GPU work before calling `resize()`, including before the first
render. The delegate cleans old size-dependent resources and creates:

```cpp
_outputImage = device->createImage({
    .size = extent,
    .format = gpu::PixelFormat::R8G8B8A8_UNORM,
    .usage = gpu::ImageUsage::Storage | gpu::ImageUsage::Sampled,
    .debugName = "RT sampled output"
});
_accumImage = device->createImage({
    .size = extent,
    .format = gpu::PixelFormat::R32G32B32A32_SFLOAT,
    .usage = gpu::ImageUsage::Storage,
    .debugName = "RT HDR accumulation"
});
```

It recomputes inverse view/projection matrices using the new aspect ratio,
updates image dimensions in camera data, and resets the sample count to zero.

The host camera block adds a background vec4 to the original fields:

```cpp
struct CameraData {
    glm::mat4 viewInverse{1.0f};
    glm::mat4 projInverse{1.0f};
    glm::vec4 misc{0.0f}; // x=sampleCount, y=width, z=height
    glm::vec4 background{0.0f};
};
```

The GLSL/Metal blocks match the 160-byte host layout. A camera Buffer ring holds
one UBO per frame slot; the matching set-0 ResourceSet ring references that
UBO and both shared images. A persistent sampled-image set for the fullscreen
pass is also recreated for the new output image, along with its graphics
pipeline targeting the surface color format.

### 7. Update sample state in the current frame slot

`update()` consumes a pending reset request, increments the sample count, and
writes camera/sample/background data to the slot that draw has waited to reuse:

```cpp
if (_controller->resetRequested)
{
    _controller->samples = 0;
    _controller->resetRequested = false;
}
++_controller->samples;
_camera.misc.x = static_cast<float>(_controller->samples);
const auto& color = _controller->color;
_camera.background = glm::vec4(color.r, color.g, color.b, color.a);
_cameraRing.at(context.frameSlot)->updateUniformBuffer(_camera);
```

The count advances only on active scene updates, independently of UI frames.
Changing the background or selecting Restart accumulation invalidates the
running mean and schedules the next scene frame. Resize also restarts it.
Refresh scene requests another sample; it does not reset the accumulated mean.

### 8. Trace rays and close the Metal compute scope

The delegate transitions both images to `General`, binds the RT pipeline and
sets, then dispatches one ray-generation invocation per output pixel:

```cpp
command.transition(_outputImage.get(), gpu::ImageLayout::General);
command.transition(_accumImage.get(), gpu::ImageLayout::General);
command.bindPipeline(_rtPipeline.get());
command.bindResourceSet(_rtPipeline.get(), 0, _rtOutputRing.at(context.frameSlot));
command.bindResourceSet(_rtPipeline.get(), 1, _sceneSet.get());
command.bindResourceSet(_rtPipeline.get(), 2, _dataSet.get());
command.traceRays(context.extent.width, context.extent.height, 1);
if (_engine->backendType() == gpu::BackendType::Metal)
{
    command.endCompute();
}
```

Metal's RT binding opens a compute encoder, so `endCompute()` must precede the
fullscreen graphics pass. Vulkan's native RT dispatch does not open that scope.

The accumulation image stays shared across frames: each sample depends on the
previous global mean. Draw submits these dispatches to the same graphics queue;
Vulkan's `General` to `General` transition emits a memory dependency, and Metal
uses tracked image hazards across ordered encoders. Camera UBOs are duplicated
because the CPU writes them while other frames can still be reading their data.

### 9. Sample output into draw's retained color

After tracing, the output transitions to `ShaderReadOnly` and is rendered with
a fullscreen triangle:

```cpp
command.transition(_outputImage.get(), gpu::ImageLayout::ShaderReadOnly);
command.beginRendering(&context.colorTarget);
command.bindPipeline(_outputPipeline.get());
command.bindResourceSet(_outputPipeline.get(), 0, _outputSet.get());
command.draw(3);
command.endRendering();
```

The output shader pair is reused from tutorial 04. It constructs positions
from the vertex index and samples the output with screen-oriented UVs.
The pass requires no mesh buffers or depth attachment. Sampling handles the
surface's native color format without an RGBA-to-BGRA byte-copy assumption.
Draw then copies retained color for presentation and composes the UI.

### 10. Pause and release resources through draw's lifecycle

UI drawing requests scene frames while active, allowing progressive refinement.
Pause retains the last result and stops tracing/sample increments; UI counters
continue advancing. Background/reset/refresh requests made while paused stay
pending until resume. Paused resize clears the replacement retained image
according to tutorial 01 and resets the samples; tracing restarts on resume.

Resize releases fullscreen pipeline/set and RT output sets before camera UBOs
and images, after GPU completion. Persistent RT resources remain unchanged.
Shutdown performs the same size-resource cleanup, then flushes the local
CleanupManager in reverse creation order: pipelines/sets before layouts and
referenced resources, TLAS before BLAS. Borrowed mesh buffers are cleaned last.

## Shader code explanation

### Path generation and iterative transport

The original sampling and geometry logic is preserved. Each pixel seeds a PCG
random generator using its coordinates and sample count, applies subpixel jitter,
and reconstructs a camera ray through inverse projection/view matrices.

For up to eight bounces, the shader traces the closest opaque triangle, adds
surface emission weighted by path throughput, and selects a new direction.
Roughness interpolates between mirror reflection and cosine-weighted hemisphere
sampling. Throughput is multiplied by albedo; negligible paths terminate early.
This is the source's educational glossy model, rather than a full material system.

Vulkan's closest-hit shader fills the hit payload and its miss shader marks
escaped rays. Metal performs intersection and analytic shading inside the kernel.
Primary misses use `cam.background.rgb`; secondary misses terminate without
adding environment illumination. The Background control represents linear
radiance and passes through the same tone mapping/gamma as the scene.

### Progressive accumulation

The first sample overwrites the mean without reading an uninitialized image:

```glsl
float frameCount = cam.misc.x;
vec3 accum = radiance;
if (frameCount > 1.0)
{
    vec3 prev = imageLoad(accumImage, ivec2(gl_LaunchIDEXT.xy)).rgb;
    accum = mix(prev, radiance, 1.0 / frameCount);
}
imageStore(accumImage, ivec2(gl_LaunchIDEXT.xy), vec4(accum, 1.0));
```

Resetting the count makes the next sample replace every accumulation texel,
so no explicit image clear is required. Floating-point storage preserves the
mean before conversion to the 8-bit display output.

### Tone mapping and channel order

The shader applies Reinhard tone mapping and gamma 2.2:

```glsl
vec3 mapped = accum / (accum + vec3(1.0));
mapped = pow(mapped, vec3(1.0 / 2.2));
imageStore(outputImage, ivec2(gl_LaunchIDEXT.xy), vec4(mapped, 1.0));
```

The GPU source writes `.bgr` for a byte copy to its BGRA swapchain. This draw
adaptation writes ordinary RGB channels because the fullscreen pass samples
an RGBA texture. Keeping the source swizzle would exchange red and blue channels
in sampled presentation.

The two ray generation shaders change only camera/background handling,
first-sample accumulation reads, and output channel order. Miss/closest-hit
behavior otherwise follows the source; four fullscreen shaders are added from
tutorial 04.

## Key concepts

- **Progressive rendering:** each active scene frame contributes one sample per
  pixel; reset when image extent or rendering inputs invalidate the mean.
- **Shared GPU history, per-slot CPU inputs:** accumulation is ordered on one
  queue; mutable camera uniforms use matching frame-slot rings.
- **Immutable scene:** BLAS/TLAS and instance metadata are built once and reused.
- **Explicit scope boundary:** Metal RT compute must close before graphics.
- **Sampled presentation:** storage output is rendered into retained color,
  leaving draw responsible for UI composition and presentation.

## Complete source

See [main.cpp](../../../examples/draw/09_ray_tracing_pipeline/src/main.cpp) and the
[shader directory](../../../examples/draw/09_ray_tracing_pipeline/shaders/).

## Building and running

The [CMakeLists.txt](../../../examples/draw/09_ray_tracing_pipeline/CMakeLists.txt)
preserves the source's shader compilation and resource packaging calls with
`draw_ray_tracing_pipeline` as the target name. The shared shader helper targets
Vulkan 1.2 for RT stages and also discovers the added fullscreen graphics
shaders. On macOS, the ray generation kernel and fullscreen shaders compile to
Metal libraries. ShaderLib loads `shaders/draw_ray_tracing_pipeline`.

Use your configured build directory, replacing `build` as needed:

```sh
cmake --build build --target draw_ray_tracing_pipeline --config Debug
```

On macOS:

```sh
./bin/macos/draw_ray_tracing_pipeline.app/Contents/MacOS/draw_ray_tracing_pipeline --backend=metal
```

On Linux, run `./bin/linux/draw_ray_tracing_pipeline --backend=vulkan`.
On Windows, run `draw_ray_tracing_pipeline.exe --backend=vulkan` under `bin/windows`.
The selected GPU/backend must pass the capability check.

To inspect the result:

1. Observe the Cornell box progressively refining as sample count increases.
2. Inspect red/green walls and mirror-like cube/sphere reflections.
3. Restart accumulation and observe the count return to one on the next update.
4. Change Background and inspect areas outside the box after accumulation restarts.
5. Pause and confirm that samples freeze while UI counters advance.
6. Resize and resume if paused to check aspect ratio and restarted accumulation.

## Next steps

- [Draw tutorial index](index.md) — Browse the completed selected example series.
- [GPU Ray Tracing Pipeline](../gpu/13_ray_tracing_pipeline.md) — Compare the lower-level path tracer.
- [RayTracingPipeline](../../api/gpu/RayTracingPipeline.md) — Pipeline description and backend behavior.
- [RayTracingScene](../../api/gpu/RayTracingScene.md) — Instance hierarchy and builds.
- [FrameResourceRing](../../api/gpu/FrameResourceRing.md) — Per-slot resource ownership.
