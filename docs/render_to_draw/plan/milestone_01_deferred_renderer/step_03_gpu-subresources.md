# M01-03 — Image views, uploads and mipmap operations

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M01-02.

## Scope and deliverable

- `lib/include/bg2e/gpu/Image.hpp`
- `lib/include/bg2e/gpu/Common.hpp`
- `lib/src/bg2e/gpu/vk/Image.cpp`
- `lib/src/bg2e/gpu/metal/Image.cpp`
- `examples/draw/06_cubemap_render_pass/src/main.cpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/gpu/Image.hpp](../../../../lib/include/bg2e/gpu/Image.hpp), [lib/include/bg2e/gpu/Common.hpp](../../../../lib/include/bg2e/gpu/Common.hpp), [lib/src/bg2e/gpu/vk/Image.cpp](../../../../lib/src/bg2e/gpu/vk/Image.cpp), [lib/src/bg2e/gpu/metal/Image.cpp](../../../../lib/src/bg2e/gpu/metal/Image.cpp), [examples/draw/06_cubemap_render_pass/src/main.cpp](../../../../examples/draw/06_cubemap_render_pass/src/main.cpp).

## Implementation work

1. Inspect existing cubemap face/mip generation before adding mechanisms. Define generic subresource ranges/views, per-mip/layer targets and sufficient layout/state tracking for ordinary textures and cubemaps.
2. Complete 2D mip targeting where current beginRendering ignores its parameter; add upload/copy/generate-mip operations with format/usage checks on both backends.
3. Choose backend convenience implementation versus exposed commands by comparative complexity. Keep native blit/mipmap encoders inside gpu; do not pretend Metal texture copy scales images.
4. Specify synchronization and final shader-read state; preserve the ShaderBinding Vulkan/Metal data model and avoid a new shader language.

## Verification and exit conditions

- A textured mesh samples a complete mip chain; face/mip cubemap results retain distinct content and valid transitions.
- Odd dimensions, single-level textures, float HDR uploads and unsupported filtering/generation formats have defined outcomes.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M01-06/M01-08 use this for texture/IBL residency. M03 BlueNoise arrays and M04/M05 readback must extend the same subresource contracts.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
