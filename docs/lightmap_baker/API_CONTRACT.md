# Proposed API and behavioral contract

This file fixes the names and observable behavior expected from the steps. The
examples are intended C++ usage, not claims that these declarations exist yet.
Implementation may add private helpers, but changes to the public names or
semantics below require an explicit plan revision.

## Public types in `bg2e::render`

```cpp
enum class LightmapMode { RTAO, RTGI };
enum class LightmapPixelFormat { RGB8, RGB32F };

struct LightmapSettings {
    uint32_t resolution = 512;          // square atlas: resolution x resolution
    LightmapMode mode = LightmapMode::RTGI;
    uint32_t accumulationFrames = 16;  // exactly this many update calls
    uint32_t samplesPerPixel = 8;
    uint32_t giBounces = 2;             // RTGI only
    float maxRayDistance = 50.0f;       // RTGI only; RTAO uses a fixed 0.1 m radius
    float giRayBias = 0.0005f;         // RTGI UV-ray origin offset in metres
    float exposureEV = 0.0f;          // RTGI RGB8 export only; multiply by 2^EV
    LightmapPixelFormat cpuFormat = LightmapPixelFormat::RGB8;
};

struct LightmapPixels {
    uint32_t width;
    uint32_t height;
    LightmapPixelFormat format;
    std::variant<std::vector<uint8_t>, std::vector<float>> rgb;
    // RGB order, tightly packed, row-major, top row first; no alpha/padding.
};

class BakerContext;             // shared scene resources; non-constructible base
class LightmapBaker;            // per-target resources and common result access
class IntegratedBakerContext;   // owns bake RT scenes, one per in-flight frame slot
class IntegratedLightmapBaker;
class StandaloneBakerContext;   // owns a production RayTracingScene
class StandaloneLightmapBaker;
class StandaloneBakeSceneAssembler;
```

`BakerContext` retains the engine pointer, scene root reference, shared pipelines,
blue noise, scene lighting/material bindings and other reusable GPU state.
`LightmapBaker` owns the target node reference, final image, per-target
UV-space G-buffer, history images, sample count and CPU result. Contexts are
created as `shared_ptr`; a baker holds a `shared_ptr` to its context, so
destroying the caller's context reference cannot leave a dangling baker.
The `Engine` and target scene must still outlive all contexts and bakers.
The baker does not own or destroy the target Drawable or its materials.
`IntegratedBakerContext` owns its bake `RayTracingScene` instances and shares
the currently prepared instance among all target bakers. It never uses
`FrameResources::rayTracingScene` as the bake TLAS. One instance per in-flight
frame slot keeps submitted builds and traces alive until that slot's fence has
completed. `FrameResources` supplies only transient command/descriptor state.

### UV-space G-buffer reuse

The baker's private `UvSurfacePass` **uses `render::GBufferManager` to own its
UV-space attachments**. It does not use a `DeferredLayer` G-buffer instance or
its camera-space formats/shaders. Add a `GBufferManager::Configuration` with
`std::vector<VkFormat> colorFormats` and `VkFormat depthFormat` (where
`VK_FORMAT_UNDEFINED` means no depth attachment), plus an additive
`GBufferManager(Engine*, Configuration)` constructor and depthless
`beginRender(VkCommandBuffer)` overload. Keep the existing constructor,
`build/resize/cleanup/accessors/transitions`, and
`beginRender(VkCommandBuffer, bool)` behavior and signatures for deferred
rendering. In the depthless configuration, `depthImage()` returns null and
`depthFormat()` returns `VK_FORMAT_UNDEFINED`; all allocation, transition,
render-begin and cleanup paths must guard the absent depth image.

`UvSurfacePass` creates one manager per target and in-flight frame slot using
four color attachments at the bake resolution: world position
`VK_FORMAT_R32G32B32A32_SFLOAT`, signed world normal
`VK_FORMAT_R16G16B16A16_SFLOAT`, zero-based submesh/material identity
`VK_FORMAT_R32_UINT`, and valid-texel mask `VK_FORMAT_R32_UINT` (0 empty,
1 covered). Material identity resolves through the submesh index. The UV
pipeline writes these attachments without depth testing; the CPU atlas
validator rejects overlapping positive-area UV triangles. Clear every
attachment to zero, then transition color writes to shader reads before
RTAO/RTGI. Read integer attachments with integer samplers and nearest
filtering. The camera G-buffer's five fixed attachment indices and depth
behavior remain unchanged. Reusing a slot or resizing its attachments
requires completion of that slot's previous GPU submission. The UV pass
retains no images from a manager after its rebuild or destruction. Standalone
baking uses slot 0 of the same private pass. Check that the selected Vulkan
formats support color attachments and sampled images before allocation; report
an explicit error if the device lacks a required format feature.

The existing `Image::cmdTransitionImage` implementation assigns
`dstStageMask` to `imageBarrier.dstAccessMask`; step 02 corrects it to use
`TransitionInfo::dstAccessMask` and verifies attachment-write to compute/ray
read synchronization. This is an internal fix with no API signature change.
The UV-specific RTAO/RTGI entry points consume the selected UV manager's
images and the context-owned TLAS directly. They do not pass that manager to
legacy camera-space methods that reconstruct position from a depth image.

`createBaker(node, settings)` validates: RT support, non-null and attached
node, `node->sceneRoot() == context.rootNode()`, standard
`scene::Drawable`, nonempty triangle submeshes, valid dimensions/settings,
and a usable UV2 atlas. The usable-UV2 check starts as a private shared
validator in integrated step 02 and becomes public `geo::UvAtlasValidator`
in atlas step 03; never maintain two divergent implementations. It returns a mode-specific baker or reports a concrete
error. The target is exactly the node's Drawable, not its descendants. Other
scene nodes remain ray-tracing occluders/light contributors. Each baker covers
all submeshes in one image. No bake method silently invokes the CPU UV2
modifier or reloads a Drawable.

### Integrated use

```cpp
auto context = std::make_shared<render::IntegratedBakerContext>(
    engine, sceneRoot);
auto baker = context->createBaker(targetNode, settings);

// In the render delegate, after the application's scene/component update:
context->prepareFrame(cmd, frameResources); // once for all bakers in this frame
baker->update(cmd, frameResources);          // one full-resolution sample

// Outside command recording, after the frame has been submitted and advanced:
auto pixels = baker->readPixels();    // waits for its last submitted update
auto image = baker->image();          // optional sampled/renderable image
```

`IntegratedBakerContext::prepareFrame(VkCommandBuffer,
vulkan::FrameResources&)` is called once per active bake frame, after the
application's scene/component update and before any baker update. It verifies
that the supplied frame is the engine's current frame slot, selects the
context-owned RT scene for that slot, collects scene bindings, and records
`RayTracingScene::update(cmd, context.rootNode())` and the required build-to-ray
barrier in the same ordered command stream. The normal renderer's TLAS build
is neither a prerequisite nor a source of bake resources. The context does
**not** call `Scene::willUpdate`, `UpdateVisitor`, `Scene::didUpdate` or resize
hooks. It may rebuild the selected slot only after the application loop has
waited for that slot's previous fence; it must release the old TLAS before
`RayTracingScene::update` overwrites its handles. A slot prepared for a frame
is shared by all target bakers in that frame; do not rebuild per target.

`IntegratedLightmapBaker::update(VkCommandBuffer,
vulkan::FrameResources&)` retains the step-01 signature. It requires a
successful `prepareFrame` with the same `Engine::currentFrame()` number,
frame-resource slot and command buffer, rejects a
detached/mismatched target root or missing TLAS/descriptor allocator, and
consumes the selected context-owned TLAS. Only transient frame descriptors
may come from `FrameResources`; neither the context nor the baker retains
`FrameResources*` or frame-owned descriptors past the recording call.
Recording an update increments the scheduled-sample count; CPU readback is
legal only after GPU submission and waits for completion. Context destruction
or scene-root replacement waits for all submitted slots (or uses the engine's
deferred cleanup). Rebuilding the same unchanged scene each frame does not by
itself reset accumulation; actual scene/target changes do. All calls for one
baker are serialized; concurrent updates are unsupported.

### Standalone use

```cpp
auto context = std::make_shared<render::StandaloneBakerContext>(
    engine, assembledScene);
context->initialize({512, 512});
context->updateScene(0.0f);          // lifecycle + one owned TLAS update

auto bakerA = context->createBaker(nodeA, settings);
auto bakerB = context->createBaker(nodeB, settings);
for (uint32_t i = 0; i < settings.accumulationFrames; ++i) {
    bakerA->update();                 // synchronous submit, one sample
    bakerB->update();
}
auto a = bakerA->readPixels();
```

`StandaloneBakerContext::initialize(VkExtent2D)` prepares frame and
descriptor resources. `updateScene(float deltaSeconds)` is the **only**
baker path that drives scene/component lifecycle. On the first call, run the
scene resize hooks at the configured extent; on each call, run
`willUpdate -> UpdateVisitor::update -> updateAll (or narrower cache refresh)
-> didUpdate`, collect lights/environment/materials, then build an owned
`render::vulkan::rt::RayTracingScene` from the complete assembled root.
This is the standalone context's own instance, distinct from both the
integrated context's per-slot instances and `FrameResources::rayTracingScene`.
Before a later `updateScene` replaces its TLAS, the context waits for prior
bake submissions and cleans the old acceleration structure and buffers.
Any draw hooks needed by a reused engine resource must be balanced; avoid
calling camera-dependent rendering hooks without a camera. A rebuild increments
a scene generation number and invalidates affected bake histories. Subsequent
`StandaloneLightmapBaker::update()` calls submit work synchronously using the
prepared generation; they do not update the scene or rebuild the TLAS. Each
successful synchronous submission advances the engine frame counter so the
shared `LightmapBaker::readPixels()` submission guard accepts the result.
Changes to scene inputs require an explicit `updateScene` before further
baking. The standalone context must work with `Engine::init()`, without a
window or an application render loop.

### Sample count and result

Each integrated `update` records one accumulation iteration for the application
to submit; each standalone `update` submits one iteration. A baker
starts at zero and becomes complete after exactly
`settings.accumulationFrames` updates. Extra updates are rejected until
`resetAccumulation()` is called. `readPixels()` returns the current
accumulated result after GPU completion; `completedFrames()` reports progress.
Settings are immutable for an existing baker. Callers reset history after
target geometry or scene changes before starting a new sequence. No temporal
reprojection using a screen camera is allowed:
history is indexed by stable UV2 texels and masked by UV coverage. No island
identifier, spatial denoising or padding fill is required by this plan;
uncovered texels remain neutral white.
All intermediate images use `resolution x resolution`. The implementation
must attempt to use FSR NativeAA at that native resolution when the required
UV-space depth/motion inputs can be supplied correctly. If they cannot, FSR
NativeAA is unavailable to this baker and the fallback is native-resolution
per-texel accumulation. Record the capability decision and reason; no upscaling is
permitted.

GPU working values remain linear float. `RGB32F` CPU output preserves them;
`RGB8` maps the final display lightmap to [0,1], clamps and rounds once at
readback. No HDR file format is added here. The image returned by `image()`
must have `COLOR_ATTACHMENT`, `STORAGE`, `SAMPLED` and
`TRANSFER_SRC` usage where supported by the chosen format. After an update it
is in `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`; consumers must wait for that
frame's submission before using it. Integrated CPU readback is allowed after
the engine advances beyond the recorded frame and waits for GPU completion.
A baker destructor releases its own
images/buffers/descriptors only after in-flight GPU use has finished or via
the engine's deferred cleanup mechanism.

## Lightmap meaning and AO material slot

The baker produces **one RGB light multiplier texture**. White is neutral.
RTAO yields a grayscale visibility factor replicated across RGB. RTGI yields
a colored indirect-light factor normalized against the scene's unoccluded
reference environment; a zero reference is handled explicitly rather than
dividing by zero. Both modes fit the same RGB8 material texture and modulate
indirect lighting only. This deliberately treats the output as a light
multiplier, not HDR irradiance; HDR irradiance export is future work.
The exact normalization must be implemented once in a shared composition
pass and verified with neutral/occluded/colored fixtures, rather than
independently in the UI and CLI.

The existing `MaterialAttributes::aoTexture` is the only material property
used. Every submesh material of the target receives **the same**
`base::Texture` file path, `aoUVSet = 1`, `aoScale = {1,1}`, and AO channel
handling consistent with RGB sampling. No new lightmap or emission property
is permitted. The material/shader path must consume the AO texture as RGB for
the baked light multiplier while still accepting existing grayscale AO images
by replicating their scalar value. Avoid sampling only
`MaterialAttributes::aoChannel` for baked RGB output. Do not apply a
direct-light multiplier to the baked indirect factor. Treat the presence of
an explicit AO texture as the internal
`HAS_BAKED_LIGHTMAP` condition (a render-uniform/GBUFFER flag, **not** a new
serialized material property). Keep the existing five G-buffer attachments
and their indices stable; add a sixth RGB attachment for the sampled baked
multiplier. The G-buffer fragment pass writes neutral white for materials
without an AO texture, and the RGB AO sample for those with one. The
composite pass uses the flag to bypass live RTGI/RTAO for that material,
then multiplies only its ambient/indirect result by the baked RGB factor.
Direct lighting and emission remain outside this multiplication. The non-RT
composite needs the same RGB multiplier behavior. Existing grayscale AO
textures still produce equal RGB factors. The serialized
AO property names remain unchanged.

## UV2 validity and atlas generation

`geo::VertexPNUUT::texCoord0` is UV1; `texCoord1` is UV2. The importers
currently substitute UV1 values when a source has no UV2, so existence of
`texCoord1` is **not** a presence test. Define a CPU `geo::UvAtlasValidator`
that rejects non-finite/out-of-range UVs, zero-area mapped triangles,
overlapping positive-area triangles and invalid index/submesh ranges,
allowing shared edges and vertices. The no-regeneration CLI skips a target
with a warning when this validator fails. This is a **usable-atlas** test:
a source with UV1 copied into UV2 but already satisfying all atlas rules can
be baked. Do not infer provenance from coordinate equality.

The planned `geo::GenerateUv2AtlasModifier` adds the complete object as one
xatlas input mesh to a **single xatlas Atlas**, using fixed output resolution
and padding. Previous UV2 values are ignored. `PackCharts` must produce
exactly one atlas image.
Reconstruct vertex and index arrays transactionally using xatlas `xref`: copy every
original attribute, including UV1 bit-for-bit, and replace only UV2.
Preserve submesh order and triangle-to-submesh mapping. If xatlas yields
multiple atlas images, invalid faces or an unusable atlas, report failure and
leave the input mesh untouched. `apply()` remains CPU-only. Editors must use
`MainLoop::safeUpdateScene` to modify loaded CPU meshes and invoke
`Drawable::reload()` after `waitIdle`; CLI modifies CPU meshes before load.

## CLI contract

`apps/lightmap_generator` accepts:

```text
lightmap_generator model  --context scene.json --model chair.bg2 --output out/
    --format png --resolution 512 --frames 32 --mode rtao
    --generate-uv2=false
    --samples-per-pixel 8
lightmap_generator prefab --context scene.json --prefab sofa.json --output out/
    [the same common options]
```

Mode and file paths are required. `--format` accepts `png`, `jpg`/`jpeg`,
`bmp`, `tga`, using `bg2e::db::ImageFormat` and extension helpers.
`--resolution`, `--frames` and `--samples-per-pixel` are positive integers.
GI bounces and max distance are validated for RTGI; RTAO uses a fixed
0.1 m occlusion radius. These mode-specific flags are omitted for RTAO;
irrelevant mode-specific flags are rejected rather than silently ignored.
No FSR scale option is exposed. Default output is RGB8. The CLI reports
one warning per skipped target, returns nonzero for invalid arguments or
fatal failures, and never writes a new context/prefab JSON.

Load the context JSON and attach model/prefab nodes **before** calling
`updateScene`. For `model`, attach one .bg2 target at the world origin.
For `prefab`, attach its complete subtree, then bake every eligible
Drawable node in a deterministic traversal. The TLAS contains context and
all target geometry, so modules can occlude one another during GI. One standalone context
and one scene update serve the entire batch. Without `--generate-uv2`,
write only images; skip invalid UV2 targets. With that flag, generate UV2
before GPU load, write an image and a new .bg2 copy for each target, and
associate its AO path in that copy. Never overwrite input model/prefab
resources, even when the output directory equals an input directory.
Output-name collisions are fatal before any output file is committed.
The modifier and validator are available from phase 2. Phase 3 creates the
executable with both `--generate-uv2` branches fully functional.

The engine-side `render::StandaloneBakeSceneAssembler` loads/attaches the
context and target subtree; `render::StandaloneBakeBatch` coordinates target
updates; `db::LightmapOutputWriter` owns output path planning and image/model
writing. The CLI entry point calls these components and contains no bake
algorithms.

## UI contract

`model_edit`: one `ModelLightmapWindow` for the active target. It offers
resolution, accumulation frames, samples per pixel and RGB
preview. Mode is fixed to RTAO. Its generated image is
written to a temporary path and assigned to AO on all submeshes, UV set 1.

`bg2e_composer`: one `SceneLightmapWindow` enumerates nodes that directly
contain a standard Drawable. The user selects one or more targets. The
window offers resolution, frames, RTAO/RTGI, samples per pixel,
GI bounces, RTGI max distance, GI ray bias and RGB8 export exposure, plus preview.
RTGI is selected by default; the displayed mode names are the full Ray Traced
Ambient Occlusion and Ray Traced Global Illumination labels. Both editors
offer power-of-two resolutions from 128 to 4096. One integrated context handles
the selection; each target has its own baker/image. Outputs go to temporary
paths and are assigned as AO on all target submesh materials.

Both windows schedule GPU bake work through their render delegates after the
application scene/component update. The delegate calls the shared integrated
context's `prepareFrame` once, then updates each selected baker using that
context-owned TLAS; it does not depend on the renderer's TLAS. The UI callback
does not record GPU commands. Scene swaps cancel pending work and release bakers after GPU
completion. A progress/blocking UI is deferred. In phase 2, the windows add
Generate UV2 and a UV1/UV2 preview. `bg2e::ui::UvMapPreview` uses the
existing `TextureWidgets` path and a `render::UvMapPreviewRenderer`
to produce a renderable/sampled Vulkan image; only UI implementation files
include `imgui.h`. Public headers and applications expose no ImGui types.
