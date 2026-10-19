# Draw offscreen application

This headless CLI renders the deferred example scene through
`app::OffscreenApplication` and `DefaultOffscreenApplicationDelegate`. It creates
no application window or UI. Camera, lights, two-submesh material textures,
transparent/environment scene support come from the deferred example; camera and
light animation are disabled so each requested sample uses a frozen view.

The executable writes `scene.png` into `--output-dir` (default
`offscreen_output`) and refuses to replace an existing file. It creates the
output directory before allocating the GPU. An error prints a diagnostic to
stderr and exits nonzero; `--help` prints options and exits successfully.

```sh
draw_offscreen_application --output-dir renders/frame --width 1280 --height 720 --scale 0.5 --frames 8 --backend vulkan
```

Supported options:

- `--output-dir DIR`
- `--width N --height N` for output dimensions; both must appear together.
- `--internal-width N --internal-height N` for a fixed internal size; both must
  appear together and cannot be combined with `--scale`.
- `--scale N` from 0.25 to 1.
- `--frames N` from 1 to 1048576; each is one completed scene sample.
- `--backend vulkan|metal`.
- `--graphics-config FILE` for the exported draw settings JSON.
- `--skip-unknown-settings` to skip unsupported scene-owned targets, printing
  every skipped property. Unknown settings reject by default.
- `--help`.

Output extent is the final image size. A fixed internal extent or scale only
changes scene rendering resolution. Explicit CLI resolution settings override
the imported graphics document; supplying both resolution forms is an error. An
explicit graphics document bypasses persisted preferences; this example does
not enable persistence. N, dimensions and backend are job/CLI settings and are
not loaded from the document. The decoder validates the imported document against
the active offscreen renderer. The current headless scene has no application
specific aliases for `lightPose/<id>` or other per-run scene-node IDs, so exports
containing these targets must use `--skip-unknown-settings` or will be rejected. Missing settings preserve live
defaults. Optional unavailable effects follow the renderer's normal fallback;
reduced internal resolution remains enabled if FSR is unavailable.

`timeStepSeconds` remains zero. Histories reset at sample zero, then AO/GI/reflection
and available FSR histories advance for every requested sample without discarded
warm-up submissions. The mean uses the completed reconstructed linear HDR scene
signals. Final tone/color and optional SMAA run once on the mean. This reference
accumulator synchronously reads each RGBA16F scene sample to CPU double precision
and uploads the mean once; high frame counts and large outputs are consequently
CPU/readback bound. The final PNG uses the explicit R8G8B8A8_UNORM surface and the
existing DB PNG writer. Only the completed accumulated result is read and saved.
The transfer is tight RGBA8 with the image's first row at the top; the PNG writer
does not flip rows or apply another color conversion, and alpha is preserved as
read. Before publishing `scene.png`, the CLI decodes its temporary PNG and checks
that dimensions and every RGBA byte match the readback. A failed write or check
removes the temporary file, and an existing final or temporary output is refused.
The readback API accepts R8G8B8A8_UNORM only; it does not convert BGRA, sRGB or
HDR images. The configured final renderer has already produced the image values
that are encoded by the PNG writer.

For a visual comparison with the interactive deferred example, use matching
renderer settings and camera/scene state, disable presentation overlays, and
compare a one-sample headless render. The headless example freezes camera/light
animation and omits editor UI, selection and gizmos; N greater than one averages
completed scene samples in linear HDR before the final color/SMAA pass, so it is
expected to differ from a single interactive frame.

The example target is `draw_offscreen_application`; the existing deferred window
application remains `draw_deferred_renderer`. Engine shaders and assets are bundled
with the executable through the project's standard example resource helper.
