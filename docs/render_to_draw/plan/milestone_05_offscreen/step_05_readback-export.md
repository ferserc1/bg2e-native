# M05-05 — Completed image readback, encoding and headless regression checks

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M05-03/M05-04 and existing DB image IO.

## Scope and deliverable

- `lib/include/bg2e/gpu/Image.hpp`
- `lib/src/bg2e/gpu/vk/Image.cpp`
- `lib/src/bg2e/gpu/metal/Image.cpp`
- `lib/include/bg2e/db/image.hpp`
- `examples/draw/offscreen_application/src/main.cpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/gpu/Image.hpp](../../../../lib/include/bg2e/gpu/Image.hpp), [lib/src/bg2e/gpu/vk/Image.cpp](../../../../lib/src/bg2e/gpu/vk/Image.cpp), [lib/src/bg2e/gpu/metal/Image.cpp](../../../../lib/src/bg2e/gpu/metal/Image.cpp), [lib/include/bg2e/db/image.hpp](../../../../lib/include/bg2e/db/image.hpp).

## Implementation work

1. Choose precise backend readback convenience versus raw image-to-host primitives based on complexity; preserve completion, row layout, subresource and float/integer/color conversion semantics.
2. Use DB low-level image writers after host data is ready. Keep native Vulkan/Metal staging implementation inside GPU and accumulated-result/encoding policy in draw/app.
3. Validate output orientation/color domain/alpha and error paths, including unwritable destination, unsupported output options and cleanup after failure.
4. Compare the headless scene with equivalent interactive configuration and record differences caused by animation/overlay/accumulation policy rather than hide them.

## Verification and exit conditions

- Independent image inspection verifies file format/dimensions, orientation and content after completion on available backends.
- A failed export has a nonzero result and releases GPU resources; concurrent/queued work is not retired before completion.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M06/M07 complete GPU-only texture scene saves using these primitives; M08 LightmapPixels exports preserve its distinct HDR and dilation semantics.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
