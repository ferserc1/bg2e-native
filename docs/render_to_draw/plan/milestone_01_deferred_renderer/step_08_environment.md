# M01-08 — Public environment conversion, IBL and sky tools

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M01-03/M01-04/M01-06.

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

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M03 GI/reflections and M04/M08 bake borrow/reuse these public objects. Scene owns environment intent, not high-level generated bindings.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-03 image handoff

Reuse `ImageSubresourceRange`, ranged attachments/resource-set views and the existing cubemap render pass. Check `FormatCapabilities::mipmapGeneration` for generated source chains and finish every sampled mip/face in ShaderReadOnly. Preserve separately authored specular roughness levels: regenerating their mip chain would overwrite those levels. Image-owned native views are borrowed and become invalid on cleanup/reallocation.

## M01-04 shader deployment handoff

Use the paired source/output paths and actual ShaderLib entry points documented in [shader-libraries.md](shader-libraries.md). The registered recursive draw shader tree already handles shared includes, native compilation and app/install/standalone deployment. Add concrete pipeline stages there; do not flatten outputs or treat the `probe` libraries as the final renderer implementation. Keep Vulkan `main`, Metal stage-specific entry points and explicit backend resource indices.

## M01-07 environment reference handoff

EnvironmentComponent has an engine-free parser and serializeReferences that retain/rebase its equirectangular source without opening or copying the image. The draw DB load path leaves this source unresolved for environment preparation here; never invoke the production cache overload to realize draw HDR/IBL. See [db-levels.md](db-levels.md).

## Implemented M01-08 handoff

Actual APIs, ownership, source replacement, linear color/irradiance/BRDF ABI and lead checks are documented in [environment-tools.md](environment-tools.md). Public tools and example 06 integration are implemented; RendererDeferred integration follows in M01-10. Synchronous generation/synchronization runs before active frame/command recording.
