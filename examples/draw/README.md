# Experimental draw examples

`bg2e::render` is the maintained Vulkan framework used by bg2 engine applications,
including its scene/material facilities. `bg2e::draw` is the experimental
multi-backend framework over `bg2e::gpu`: it supplies GPU-based scene delegates
and retained scene/UI composition while its high-level API can evolve. These
labels describe API maturity and scope, not Debug/Release builds or deployment
servers. Both frameworks are supported; render is not deprecated.
These examples use the same Application/MainLoop entry points with the explicit
EngineConfig overload. They do not initialize Factory or construct a private
execution wrapper.

## 01: Window and UI

The `draw_window_ui` target creates a window, clears the retained scene image
through abstract GPU commands and shows the ImGui demo through `bg2e::ui`.
It uses the existing SDL application bundle/output conventions and requires no
scene shaders. Executables are placed under `bin/<platform>`; on macOS the
executable is inside `draw_window_ui.app/Contents/MacOS/`.

With no arguments, EngineConfig selects Metal on macOS and Vulkan elsewhere.
Pass `--backend=vulkan` or `--backend=metal` to override it. Metal requires macOS.
Unknown arguments and repeated backend selection fail before window/GPU
allocation with a descriptive error. There is no interactive startup prompt.

The control window provides background color, scene pause/resume and refresh.
Scene and UI counters show that presentation/UI continue without scene redraws.
Color changes invalidate the scene and request presentation. While paused, the
last valid scene remains visible and changes/refresh requests stay pending until
resume. Resize must replace the retained image; while paused, its initial clear
uses the current pause clear color. Resuming draws the currently selected color.

`requestFrame()` requests presentation only. `requestSceneFrame()` invalidates
the scene and requests presentation. Scene controls run on the main thread;
worker changes should be queued through `safeUpdateScene()`. Async loading uses
the same pause path, displays Loader/UI and resumes through main-thread completion.

Submission is asynchronous. Acquisition waits only for reuse of an occupied
frame slot, preserving frames in flight. Global waitIdle tracks all registered
submissions across queues; callers coordinate producers for resize/teardown.
Deferred cleanup uses completion records independent of reusable command/fence
wrappers. Objects should own persistent resources indexed by frame slot rather
than allocating/destroying resources or descriptors on every frame.

## 02: Simple triangle

The `draw_simple_triangle` target adapts `gpu/05_simple_triangle` to the
Application/MainLoop and `draw::RenderLoopDelegate` lifecycle from example 01.
Like the original GPU example, its indexed mesh is a pentagon made of five
triangles, with a procedural 2x2 texture and an animated tint passed through
fragment push constants. A compute shader generates the background gradient.
The information window remains available; the ImGui demo window is omitted.

The retained draw color target has no depth attachment and does not support
storage writes. Compute therefore writes to a separate RGBA8 storage/sampled
image. A fullscreen textured quad draws that image into the retained target,
followed by the pentagon in the same graphics pass. The Background picker tints
the gradient. Setting it to white shows the original gradient colors.

The UI requests scene frames while animation is active. Pause freezes both the
retained image and scene animation time while the UI continues updating.
Background changes and Refresh scene requests remain pending until resume.
Resizing while paused follows example 01's clear-color behavior.

Resources are created during scene initialization and resize, rather than per
frame. Descriptor bindings remain unchanged between resizes, so the same sets
can be reused across frame slots. The gradient image and graphics pipeline are
recreated after draw drains in-flight work on resize; cleanup also runs after
GPU completion. Command submission, presentation and UI composition belong to
draw, not to the example delegate.

Backend selection uses the same `--backend=vulkan` and `--backend=metal`
arguments as example 01. Both GLSL and Metal shader sources are included.
ShaderLib loads `shaders/draw_simple_triangle` using `main` for SPIR-V and
`vertMain`/`fragMain`/`compMain` for Metal. The executable is placed under
`bin/<platform>`; on macOS it is inside
`draw_simple_triangle.app/Contents/MacOS/`.

## 03: Uniform buffers

The `draw_uniform_buffers` target adapts `gpu/07_uniform_buffers` to draw's
managed application lifecycle. It renders a rotating textured cube over a
solid background, with the same information, background, pause/resume, and
refresh controls as example 02. There is no ImGui demo window.

Set 0 contains a persistent camera UBO, set 1 contains a model UBO duplicated
with matching Buffer/ResourceSet FrameResourceRings, and set 2 contains the
persistent texture and sampler. Model updates and bindings select the same
`FrameContext::frameSlot`, after draw has waited for slot reuse.

The delegate owns a depth attachment because draw provides only retained color.
Resize runs after GPU completion, updates the camera aspect ratio, recreates
the depth attachment and graphics pipeline, and rebuilds the model rings using
the current in-flight frame count. A local CleanupManager owns persistent
DeviceResource teardown; the mesh, depth, pipeline, and rings are cleaned
explicitly after draw drains GPU work.

The cube rotates only while the scene is active. Pause preserves the retained
image and freezes animation time while the UI continues updating. Backend
selection, shader compilation, packaging, and paused-resize behavior follow
the preceding examples. Shader resources are under `shaders/draw_uniform_buffers`.

See [Tutorial 03](../../doc/tutorials/draw/03_uniform_buffers.md) for the complete
implementation walkthrough.

## 04: Render to texture

The `draw_render_to_texture` target adapts `gpu/08_render_to_texture`: it renders
a rotating textured cube into an owned RGBA8 color/depth target, applies a Sobel
compute filter into another RGBA8 image, copies the result back to the color
image, and samples it into draw's retained target with a fullscreen triangle.
This preserves the image-copy demonstration without requiring TransferDst or
Storage usage on the retained draw target. Sampling also handles its native
color format. The unfiltered image is reused after compute finishes, keeping
three size-dependent images rather than allocating a fourth copy destination.

The example retains the camera/model UBOs, model resource rings, and local
CleanupManager from example 03. Resize updates the camera aspect ratio and
recreates images, image sets, rings, and graphics pipelines after GPU completion.
The Sobel shaders clamp neighborhood coordinates at borders on both backends.

The information window keeps background, pause/resume, and refresh controls.
Background changes the input clear color and therefore edge contrast; Sobel
produces grayscale edges with dark uniform regions. Pause freezes the processed
image and rotation while the UI remains active. The UI is composed after the
filter, so its controls and text remain unaffected.

Backend selection and shader packaging follow the preceding examples.
Resources are packaged under `shaders/draw_render_to_texture`.
See [Tutorial 04](../../doc/tutorials/draw/04_render_to_texture.md) for the full
implementation walkthrough.

## 05: Cubemap

The `draw_cubemap` target adapts `gpu/09_cubemap`: it loads
`autumn_field_4k.hdr`, renders an inward-facing textured sphere into six 512x512
RGBA16 floating-point cubemap faces, and renders a rotating mirror cube into
draw's retained color target with an owned depth attachment. Both shader pairs
and the original face camera convention are preserved.

The UI keeps background, pause/resume, refresh, and scene/UI counters. Direct
cubemap debug switches between reflection and sampling by local cube direction;
releasing Space toggles the same setting when the UI does not capture keyboard
input. Changes made while paused remain pending until resume.

Sphere model, cube model, and mode settings use Buffer/ResourceSet rings indexed
by `FrameContext::frameSlot`. Resize updates the camera aspect ratio, rebuilds
rings, and replaces the scene depth and reflection pipeline after GPU completion.
The fixed-resolution cubemap and conversion pipeline remain persistent. Like
the source example, all six faces are rendered on every active scene update.
Persistent resources use a local CleanupManager for ordered teardown.

Shaders are packaged under `shaders/draw_cubemap`, with the same Vulkan/Metal
build conventions as the preceding examples. See
[Tutorial 05](../../doc/tutorials/draw/05_cubemap.md) for the complete walkthrough.

## 06: Cubemap render pass

The `draw_cubemap_render_pass` target adapts `gpu/10_cubemap_render_pass`.
During initialization, `gpu::CubemapRenderPass` converts the HDR environment
into a 1024x1024 cubemap, generates a 32x32 diffuse irradiance map, and renders
a specular reflection mip chain from 1024x1024 down to 8x8. All outputs use
RGBA16 floating-point format. Each specular mip has a separate roughness UBO
and resource set, preserving its parameters across the recorded submission.

The active scene draws only a rotating reflective cube. Its model uses matching
Buffer/ResourceSet rings indexed by `FrameContext::frameSlot`. The delegate owns
depth; resize updates the camera and rebuilds the visible pipeline, depth, and
model rings after GPU completion. Generated cubemaps remain unchanged.

The information window selects Original environment, Diffuse irradiance, or
Specular reflection, and cycles the specular mip. Space release cycles maps;
Enter release cycles mips when scene input receives keyboard events. Background,
pause/resume, refresh, and counters follow preceding examples. Selections made
while paused become visible on resume. Irradiance is displayed through the
reflection shader as a map preview, rather than a complete PBR material.

Shaders follow the source GPU example, with packed Metal camera-position push
constants to match the host's 16-byte camera/LOD block. Shader compilation and
packaging use the same CMake helpers and the `shaders/draw_cubemap_render_pass`
resource path. See [Tutorial 06](../../doc/tutorials/draw/06_cubemap_render_pass.md)
for the implementation walkthrough.

## 07: Ray query shadows

The `draw_ray_query_shadows` target adapts `gpu/11_ray_query_shadows`. It
rasterizes a plane, red cube, and blue sphere with Lambert lighting, then queries
the scene from each fragment toward an orbiting point light to create hard
shadows. It checks the selected device's ray tracing support before loading
scene shaders and exits with a descriptive error when support is unavailable.

Static raster meshes supply geometry for one RayTracingMesh per submesh. BLAS
are built once; RayTracingScene instances are rebuilt before each active scene
draw, preserving the source's dynamic TLAS demonstration. The TLAS, light UBO,
and matching resource set use three FrameResourceRings indexed by the same
`FrameContext::frameSlot`, isolating mutable resources across frames in flight.
Each TLAS is initially built before its resource set binds it.

The delegate owns depth and updates camera projection on resize, after draw
drains GPU work. It recreates the pipeline, depth, and slot rings while keeping
static geometry and BLAS. Cleanup releases sets and TLAS before BLAS, then
releases the borrowed mesh buffers. The UI retains background, pause/resume,
refresh, and counters, and displays the selected device. Pause freezes light
animation and scene work while UI presentation continues.

All four shaders preserve the original GPU example. CMake retains its explicit
Vulkan 1.2 graphics shader compilation required for fragment ray queries, plus
Metal compilation and packaging under `shaders/draw_ray_query_shadows`.
See [Tutorial 07](../../doc/tutorials/draw/07_ray_query_shadows.md) for the full
walkthrough and platform support requirements.

## 08: Deferred cleanup

The `draw_deferred_cleanup` target adapts `gpu/12_deferred_cleanup`. It alternates
a rotating textured cube and sphere every five seconds of active scene time.
Replaced MeshPU wrappers are moved into shared ownership captured by an engine
CleanupManager deferred closure. Each closure cleans its retired vertex/index
buffers after captured submission completion records become ready.

Draw already polls the engine manager after every presentation, including UI
frames with a paused scene. The local manager handles persistent resources;
model UBO/resource-set rings use `FrameContext::frameSlot`. Resize and shutdown
drain deferred closures after draw has waited for GPU completion. The delegate
owns depth and recreates its pipeline, depth, and rings on resize.

The information window adds current geometry, switch/pending/release counts,
and Switch geometry alongside background, pause/resume, refresh, and frame
counters. Pause freezes rotation and automatic switching; manual requests stay
pending until resume, while already scheduled cleanup can continue. Creation
and release messages are logged to the console. Mesh uploads remain synchronous,
as in the source, so pending release counts may return to zero quickly.

The four shaders and CMake compilation/packaging calls preserve the GPU example,
with resources under `shaders/draw_deferred_cleanup`. See
[Tutorial 08](../../doc/tutorials/draw/08_deferred_cleanup.md) for the walkthrough.

## 09: Ray tracing pipeline

The `draw_ray_tracing_pipeline` target adapts `gpu/13_ray_tracing_pipeline`.
It preserves the Cornell box, mirror-like cube/sphere, emissive ceiling light,
and iterative eight-bounce path tracer. BLAS, TLAS, and instance metadata are
built once. The selected device must pass the ray tracing capability check.

Each active scene update contributes one sample per pixel to an RGBA32F running
mean and tone maps the result into an RGBA8 storage/sampled image. A fullscreen
triangle samples that image into draw's retained target, which does not support
storage writes or transfer-destination copies. Camera UBO/output-set rings use
the same `FrameContext::frameSlot`; the shared accumulation image is ordered
across submissions to draw's graphics queue. The Metal RT compute scope is
explicitly closed before the fullscreen graphics pass.

The information window includes device/backend, accumulated sample count,
Restart accumulation, background, pause/resume, refresh, and frame counters.
Background controls linear radiance for primary camera rays that miss geometry;
secondary escaped paths remain black. Background changes, manual restart, and
resize reset accumulation. Pause freezes tracing while UI presentation continues.
Resize updates camera aspect ratio and replaces images, rings, and the fullscreen
pipeline after GPU completion. Cleanup releases TLAS before BLAS and mesh buffers.

The ray generation shaders add background input, avoid reading accumulation on
the first sample, and write RGB instead of the source's BGRA-copy swizzle.
Fullscreen shaders follow example 04. CMake preserves the source's shader helper
and packaging conventions under `shaders/draw_ray_tracing_pipeline`.
See [Tutorial 09](../../doc/tutorials/draw/09_ray_tracing_pipeline.md) for the
complete implementation walkthrough.

## Deferred renderer product (M01)

The `draw_deferred_renderer` target in `draw/10_deferred_renderer` is the first
high-level deferred scene over `gpu` on Vulkan/Metal. It uses the non-template
DefaultRenderLoopDelegate, RendererDeferred, scene components and draw Drawable;
the production `17_deferred_renderer` remains a separate regression reference.

The scene retains the textured two-submesh OBJ (ten provided material maps),
a nested opaque model and deep clone, both ordinary and solid transparency,
floor, HDR environment, orbiting perspective camera and four rotating lights.
It adds an alpha-checker canopy and a packed-channel/UV1 cube. The original
reference's ten lights are reduced to four to respect draw's eight-light ABI.
Default internal scale is 67%, using single-sample bilinear reconstruction with
one final color operation. FSR is optional; unavailable SDK/backend/device keeps
reduced-resolution output. Advanced AO/GI/reflections, engine accumulation,
denoise, baking and manipulation are intentionally absent until their milestones.

```sh
# Executable under bin/<platform>; on macOS use its app bundle executable.
draw_deferred_renderer --backend=vulkan
draw_deferred_renderer --backend=metal --upscaler=fsr
draw_deferred_renderer --no-rt --upscaler=application
draw_deferred_renderer --upscaler=fsr --settings=/path/to/graphics.json
```

Options are `--backend=vulkan|metal`, `--upscaler=bilinear|fsr|application`,
`--no-rt`, `--validation`, `--settings=FILE` and `--restore-preferences`.
No backend prompt is used. Unknown/repeated options fail before GPU allocation.
`--help` prints the options. Metal requires macOS. No-RT fixture mode disables
Drawable tracing geometry, light shadow requests and the shadow setting; it does
not pretend to change the hardware's reported query capabilities. Native no-RT
hardware also uses the supported ordinary lighting fallback.

The scene panel provides pause/refresh/animation, upscaler choice, same-type
renderer replacement, stale-edit rejection, clone tint/roughness, glass opacity,
transparent visibility, cutout visibility/threshold and packed-channel selection/
inversion. CPU values are read from actual objects and material edits are queued
at a drained boundary. Clone initialization checks independent CPU/GPU mesh,
material, texture/image/sampler ownership and applies a distinct tint. Injected
CountingGBuffer displays actual opaque/transparent pass counts; the application
resampler supplies live reflected controls without engine registration.

GraphicsSettingsWindow reads live reflected final/debug/sky/environment/light/
shadow/resolution/upscaler settings, imports/exports exact typed settings files
and explicitly saves/restores optional preferences. No settings restore happens
implicitly. With both startup inputs, `--settings=FILE` takes precedence over
`--restore-preferences`. Use a document matching the selected upscaler; strict
unknown/unavailable/type diagnostics are displayed. Renderer replacement retains
the scene, final layer/settings, shadow settings, scale policy and counters;
old queued actions are rejected by generation. UI presentations while paused do
not advance scene animation or SDK temporal state.

Engine shader libraries and existing assets are deployed by bundle_app_sdl;
there are no private copies of PBR/environment/query/upscale algorithms or new
application shaders. The target follows existing platform bundle/output and
packaging conventions. Agent work does not establish compilation/runtime success:
see the [manual acceptance checklist](10_deferred_renderer/README.md) and
[implementation handoff](../../docs/render_to_draw/plan/milestone_01_deferred_renderer/working-example.md).

## 12: Integrated lightmaps

`draw_lightmaps` generates independent UV2 atlases, bakes GI/AO and applies
completed RGB maps with live indirect passes disabled. It includes settings,
progress/cancel/restart, comparison and UV/result previews. See the
[example guide and manual checks](12_lightmaps/README.md).
