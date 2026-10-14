# M02-02 — Picking graphics and minimally sufficient GPU readback

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M02-01 and M01 primitive/binding APIs.

## Scope and deliverable

- `lib/src/bg2e/manipulation/SelectionManager.cpp`
- `lib/include/bg2e/gpu/Image.hpp`
- `lib/include/bg2e/gpu/CommandBuffer.hpp`
- `lib/src/bg2e/gpu/metal/Image.cpp`
- `shaders/src/draw/picking/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/src/bg2e/manipulation/SelectionManager.cpp](../../../../lib/src/bg2e/manipulation/SelectionManager.cpp), [lib/include/bg2e/gpu/Image.hpp](../../../../lib/include/bg2e/gpu/Image.hpp), [lib/include/bg2e/gpu/CommandBuffer.hpp](../../../../lib/include/bg2e/gpu/CommandBuffer.hpp), [lib/src/bg2e/gpu/metal/Image.cpp](../../../../lib/src/bg2e/gpu/metal/Image.cpp).

## Implementation work

1. Reimplement the identifier/depth pick pass with draw geometry and explicit shader bindings. Preserve encoded RGBA8 baseline, pick kinds, stable per-snapshot ID mapping and unjittered view matrices.
2. Assess existing readPixelsRGBA8 versus subregion/raw copy primitives by implementation complexity. Add only necessary GPU operations on both backends; native copies/readback handlers stay in GPU.
3. Define logical-window/drawable-pixel/DPI/viewport/Y conversion and synchronously complete the pick before mapping its result. Async readback is not required for parity.
4. Preserve click versus drag, additive/empty pick behavior and type/transform-gizmo priority as supported by current implementation. Do not promise unsupported node picking without implementing it.

## Verification and exit conditions

- Pick exact submeshes at multiple scales/DPI values and viewport offsets; blank/removed objects produce empty rather than stale selections.
- Query shadows, FSR/default scale and ordinary no-RT rendering remain usable during picks.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M02-06 viewpoint measurements reuse ID/depth primitives; M06/M07 selection widgets use the resulting stable map/lifetime contract.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
