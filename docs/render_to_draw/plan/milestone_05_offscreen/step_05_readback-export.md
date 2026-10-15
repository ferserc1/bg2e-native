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
3. Validate output orientation/color domain/alpha and error paths, including unwritable destination, unsupported output options and cleanup after failure.
4. Compare the headless scene with equivalent interactive configuration and record differences caused by animation/overlay/accumulation policy rather than hide them.

## Verification and exit conditions

- Independent image inspection verifies file format/dimensions, orientation and content after completion on available backends.
- A failed export has a nonzero result and releases GPU resources; concurrent/queued work is not retired before completion.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M06/M07 complete GPU-only texture scene saves using these primitives; M08 LightmapPixels exports preserve its distinct HDR and dilation semantics.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-03 primitive readback baseline

`gpu::Image::readPixels(bytes, range, finalLayout)` already provides synchronous packed color readback for a single full mip/layer, including HDR/integer formats. Reuse or extend that API for the required output/region/completion semantics; do not duplicate native staging helpers outside gpu. Export conversion, accumulated output selection and format/encoding orchestration remain this milestone's work.
