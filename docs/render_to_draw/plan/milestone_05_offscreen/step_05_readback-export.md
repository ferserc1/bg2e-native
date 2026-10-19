# M05-05 — Completed image readback, encoding and headless regression checks

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M05-03/M05-04 and existing DB image IO.

## Scope and deliverable

- `lib/include/bg2e/gpu/Image.hpp`
- `lib/src/bg2e/gpu/vk/Image.cpp`
- `lib/src/bg2e/gpu/metal/Image.cpp`
- `lib/include/bg2e/db/image.hpp`
- `examples/draw/13_offscreen_application/src/main.cpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/gpu/Image.hpp](../../../../lib/include/bg2e/gpu/Image.hpp), [lib/src/bg2e/gpu/vk/Image.cpp](../../../../lib/src/bg2e/gpu/vk/Image.cpp), [lib/src/bg2e/gpu/metal/Image.cpp](../../../../lib/src/bg2e/gpu/metal/Image.cpp), [lib/include/bg2e/db/image.hpp](../../../../lib/include/bg2e/db/image.hpp).

## Implementation work

1. Reuse existing synchronous gpu::Image::readPixels for one initialized color mip/layer (TransferSrc, tight raw-format bytes; Metal staging padding is stripped). readPixelsRGBA8 is limited to R8G8B8A8_UNORM, not BGRA/sRGB/HDR conversion. Add only missing GPU primitives; keep conversion, orientation, color-domain/alpha and encoding decisions explicit above staging.
2. Use DB low-level image writers after host data is ready. Keep native Vulkan/Metal staging implementation inside GPU and accumulated-result/encoding policy in draw/app.
3. Validate RGBA byte count before encoding. Write to a PNG-suffixed temporary path, decode the encoded file and verify dimensions and exact RGBA bytes before publishing the final path; remove the temporary file on failure and refuse pre-existing final/temporary outputs.
4. The backend readback copies rows in image order; the current Vulkan viewport uses a negative-height viewport from the upper edge and Metal uses its upper-origin viewport. The PNG writer keeps that top-to-bottom row order, adds no color conversion and preserves the read alpha. Independently inspect asymmetric scene content/orientation on each available backend.
5. Compare a one-sample headless render with equivalent interactive renderer/camera/scene settings and presentation overlays disabled. Record that camera/light animation and editor UI/selection/gizmos are absent; N greater than one averages completed linear-HDR samples before the final color/SMAA pass.

M02's picking and viewpoint utilities use their own synchronous RGBA8/D32
readbacks; they do not supply the completed output image or its encoding policy.
When comparing output, state whether configured final overlays were recorded and
ensure no selection/gizmo/editor preview is silently included in a scene-only
export. See [M02 review](../milestone_02_manipulation/review.md).

## Verification and exit conditions

- Independent image inspection verifies file format/dimensions, orientation and content after completion on available backends.
- Encoded PNG round-trip verifies exact dimensions and RGBA bytes; failed write/verification leaves no temporary output and returns nonzero.
- An unwritable destination fails without publishing an image; concurrent/queued work is not retired before completion.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M06/M07 complete GPU-only texture scene saves using these primitives; M08 LightmapPixels exports preserve its distinct HDR and dilation semantics.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-03 primitive readback baseline

`gpu::Image::readPixels(bytes, range, finalLayout)` already provides synchronous packed color readback for a single full mip/layer, including HDR/integer formats. Reuse or extend that API for the required output/region/completion semantics; do not duplicate native staging helpers outside gpu. Export conversion, accumulated output selection and format/encoding orchestration remain this milestone's work.

## M03-10 reviewed implementation requirement

Validate N=1 and controlled N-sample HDR average before one final color/optional SMAA operation and typed completed readback. Cover disabled/unavailable processors, sRGB/UNORM conversion and headless import of requested-but-unavailable effect values separately from strict required-job capability failure.

Source and validation boundaries: [M03 technical review](../milestone_03_ray_tracing/review.md). Include these conditions in this step's implementation and exit checks.


## M04-04 actual output handoff

Bake now has strict completed RGBA32F readback and convertRgba32 -> LightmapPixels RGB8/RGB32F. Reuse its explicit top-row/UV-v=0 layout and linear codec for bake export: GI exposure applies only to RGB8; no gamma, clipping float HDR or receiver material multiplication. The GPU texture and CPU pixels share the same chart padding. This does not supply an asynchronous staging primitive or define offscreen camera-image codecs implicitly.

See [output implementation](../milestone_04_lightmaps/accumulation-output.md).


## M04-07 reviewed implementation requirement

Validate raw RGBA32F transfer length and explicit conversion before generic DB encoding. Bake LightmapPixels is linear RGB, row zero UV-v=0; RGB8 GI exposure is a bake CPU conversion policy, not a universal scene-tone-map policy. Test marked UV2 RGB maps through the updated RGBA16F G-buffer and preserve custom-layer format validation.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).

## M05-03 concrete integration

Use [implemented accumulation/settings](accumulation-settings.md). Populate
DrawOffscreenApplicationConfig.frames, graphicsConfig, optional
internalExtentOverride or scaleOverride in initConfig; the app owns validation,
strict parsing, stopped live import and the exact completed-sample driver.
DefaultOffscreenApplicationDelegate owns the renderer and averaging; do not add
another loop, EMA or final-color average. Extend its catalog for example-owned
lightPose IDs or report rejection/explicit skip diagnostics. Disable frame-based
animation for the default frozen scene. Export/read only when hasResult() is true
inside the last didRenderFrame callback, while its surface/device remain alive.
Early callbacks are per-sample notifications with no exportable accumulated result.
Exactly N scene submissions plus one final-only conversion are intentional;
readback/upload/helper submissions do not add scene samples. N=1 still converts
once. Account for the reference CPU/readback cost in performance descriptions.


## M05-04 scene CLI handoff

The registered target is `draw_offscreen_application`; it writes exactly
`<output-dir>/scene.png`, creates the directory before GPU startup, rejects an
existing output and reports a nonzero failure. The completed surface is explicit
R8G8B8A8_UNORM and is read synchronously in the last completed callback before
engine teardown. M05-05 still owns validation of row orientation/content and an
independent inspection of the encoded PNG on supported platforms.
