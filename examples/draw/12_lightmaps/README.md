# Integrated lightmap example

`draw_lightmaps` demonstrates `bake::IntegratedBakerContext` with the draw
renderer. It creates a static room with a floor, a white cube, an independent
red clone, colored walls, a transparent display-only object and an emission
reference. Direct lights, environment/IBL and emission remain enabled. Live
indirect AO/GI and live reflections start disabled and have no editable renderer
settings window in this example.

Use **Generate UV2 and bake / restart** to generate independent UV2 atlases on
fresh drawable realizations and bake floor/cubes sequentially. The source
meshes/materials remain available for clean retries. The actual LightmapSettings
object supplies mode (GI or AO), resolution, samples, bounces, bias, distances,
dilation and CPU output format. Defaults are 128 square, 16 accumulated updates,
4 rays per update and RGB32F CPU output. Settings editing is hidden during a job;
edit them after cancellation or completion and start again.

The application submits one synchronous sample per `prepareFrame`, before
window acquisition. Both GPU backends wait for that submission inside
`Device::immediateSubmit`; only its successful return is followed by
`completeFrame`. Progress counts completed samples. UI input is handled between
samples; a costly sample can delay input. This example deliberately trades
pipelining for an explicit completion boundary and does not require a new
post-submit render-loop callback. Cancel drops partial results and leaves the
scene usable. Missing RT support, incompatible profiles or failed validation
are reported in the controls while ordinary scene display continues.

After every target completes, the application captures each typed CPU result,
releases the bake context, validates the GPU map's display range and adopts all
maps. GPU RGBA32F results are retained directly by draw textures; RGB8 CPU export
selection/exposure does not quantize or expose the live display map. The shader
interprets the AO texture slot explicitly as a baked RGB multiplier using UV2,
updates live and stored material metadata, and bypasses scalar AO. Baked GI
scales diffuse IBL/sheen while preserving specular IBL; baked AO scales all
indirect IBL. Direct light and emission remain separate. Toggle **Apply baked
indirect maps** for a before/after comparison. Original scalar AO and a baked
map cannot coexist in this six-slot material representation.

Use **Next target**, **UV1** and **UV2** for the UV diagnostic. Before the first
bake, default cube UV2 can overlap. Atlas generation replaces the source of the
preview, and resolution changes followed by another bake regenerate the atlas.
The second preview shows the latest completed multiplier for the selected
target. Both UI previews retain resource owners and re-register replaced images.

The HDR G-buffer multiplier uses RGBA16F (32 total color attachment bytes per
sample). Maps exceeding 65504 are rejected for display rather than clamped;
RGB32F CPU output and final bake images keep their full precision. The example
does not save maps or scene files. GPU-only material sources must be materialized
by the future export workflow; serializing sampler metadata alone is not export.

The normal examples build registers `draw_lightmaps` using the shared bundle
helper and the engine's existing recursive native shader deployment. No extra
assets or private bake shaders are needed. Run the executable from
`bin/<platform>` after the project lead builds it. Backend/build/UI results have
not been recorded by the implementing agent; use the manual checks below.

## Project lead checks

- On Vulkan and Metal, compare unbaked and baked GI with live indirect AO/GI
  disabled. Check colored diffuse bounce/occlusion, HDR values above one,
  unchanged direct light/emission/specular IBL, and GI versus AO mode.
- Repeat with RGB8 and RGB32F CPU formats, sample counts 1 and 16, several atlas
  resolutions and dilation zero/nonzero. Verify progress reaches exact counts,
  white unbaked areas, clean charts and stable original versus cloned geometry.
- Cancel midway through a target and between targets, then change settings and
  restart. Verify partial maps are never adopted and completed UI textures are
  rebound after reset/resolution/target/UV changes. Close the window during a
  job, after completion, and after failed RT/profile preparation.
- On unsupported RT hardware, verify scene/UV diagnostics remain usable and
  the job reports its reason without terminating the application.
- Rebuild existing draw examples and both native shader families. Existing
  scalar AO materials must keep their behavior; unmarked AO textures must never
  become RGB lightmaps merely because a texture is present or uses UV2.


## Missing bake shader library after an incremental build

The engine now explicitly compiles both draw and bake native shader trees. If a
previous build reports a missing `bake/uv_surface/surface.vert.metallib`, reload
CMake and rebuild `draw_lightmaps`. Its shader dependency generates the bake
stages and refreshes both trees inside the app resources, including when only a
shader changes. Check the newly rebuilt `draw_lightmaps` bundle when retrying.
