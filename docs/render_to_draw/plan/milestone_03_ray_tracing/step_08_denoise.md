# M03-08 — Public scalar/HDR denoise and reusable surface inputs

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M03-04–M03-07.

## Scope and deliverable

- `lib/src/bg2e/render/deferred/DenoiseFilter.cpp`
- `lib/include/bg2e/draw/pipeline/DenoiseFilter.hpp`
- `shaders/src/draw/denoise/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/src/bg2e/render/deferred/DenoiseFilter.cpp](../../../../lib/src/bg2e/render/deferred/DenoiseFilter.cpp).

## Implementation work

1. Implement compatible scalar/HDR bilateral denoise with kernel radius, depth/normal thresholds and sigma accessors. Preserve ordering relative to temporal/effect/composition outputs.
2. Expose shader/configuration/resource replacement and explicit depth/normal/surface input requirements for application renderers and later bake reuse.
3. Ensure dispatch dimensions, storage formats, image borders and sampled/storage state transitions are correct in explicit GLSL/Metal implementations.
4. Avoid a hard-wired dependency on the camera GBuffer class where a reusable surface profile suffices; choose no filtering when unsupported rather than a wrong-format output.

## Verification and exit conditions

- Noise reduces without smearing declared depth/normal discontinuities or corrupting HDR energy; odd extents and borders are valid.
- A compatible custom filter can be supplied and settings enumeration reads its actual accessor values.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M04 decides whether/how bake accumulation uses these filters based on its UV profile. Do not duplicate private kernels to bypass incompatible inputs.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
