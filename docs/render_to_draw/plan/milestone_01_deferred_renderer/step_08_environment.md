# M01-08 — Public environment conversion, IBL and sky tools

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M01-03/M01-04/M01-06.

## Scope and deliverable

- `lib/include/bg2e/render/EnvironmentResources.hpp`
- `examples/draw/06_cubemap_render_pass/src/main.cpp`
- `lib/include/bg2e/draw/environment/EnvironmentResources.hpp`
- `lib/include/bg2e/draw/environment/CubemapRenderer.hpp`
- `lib/include/bg2e/draw/environment/BRDFIntegrationMapTextureGenerator.hpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/render/EnvironmentResources.hpp](../../../../lib/include/bg2e/render/EnvironmentResources.hpp), [examples/draw/06_cubemap_render_pass/src/main.cpp](../../../../examples/draw/06_cubemap_render_pass/src/main.cpp).

## Implementation work

1. Reimplement CubemapRenderer, sphere conversion, irradiance convolution, specular prefilter and SkyboxRenderer in draw::environment using existing GPU face/mip support.
2. Provide reusable BRDF LUT generation/resources, explicit samplers/color interpretation and configurable generation shaders/objects. Keep these public standalone services, separate from pipeline internals.
3. Update declarative scene environment changes and blur without native descriptors in scene; preserve default sky generation and HDR source behavior.
4. Use coordinated safe replacement for environment textures and restore shader-read states. Do not gate IBL on RT availability.

## Verification and exit conditions

- A standalone environment generation call and deferred scene share valid irradiance/prefilter/LUT outputs on Vulkan and Metal.
- Environment/blur switching, missing HDR input and resize produce defined behavior with no-RT devices.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M03 GI/reflections and M04/M08 bake borrow/reuse these public objects. Scene owns environment intent, not high-level generated bindings.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
