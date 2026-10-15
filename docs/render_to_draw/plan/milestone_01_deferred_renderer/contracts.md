# M01-01 public contracts

These are implemented API contracts, not a working deferred renderer or a milestone review.

## Public API and ownership

- `draw::Renderer` in `draw/Renderer.hpp` supplies initialize, attachScene, resize, update, record and cleanup. The non-template DefaultRenderLoopDelegate exclusively owns it. Concrete renderers hold their live `RendererConfiguration`; the accessor returns that state rather than a preferences copy.
- `draw::pipeline::RenderLayer` is the public base for compatible G-buffer, illumination, shadow and final-processing objects. `LayerSelection` retains a shared component and its resolved `LayerConfiguration`. An application can transfer unique ownership into that shared pointer. There is no implicit borrowed component convention. A pass instance participates in one renderer lifecycle; sharing ownership does not authorize concurrent initialization or recording across renderers.
- `PassContract` declares an ordered input/output ABI: stable semantic names, image formats/types, usage and internal/output extent domains. `shaderInterface` identifies resource bindings, stages and packing. Concrete components own their shader choices and expose their actual contract. Application replacements must supply the same ABI or explicitly construct a different compatible graph.
- `Renderer::recordLayer` checks the selected contract and actual image bindings before invoking the injected layer's virtual `record`. Derived renderers resolve graph resources and use this dispatch path. There is no concrete graph executor yet (M01-10).
- `draw::environment::EnvironmentResources` retains sky, irradiance, prefiltered specular, BRDF and sampler GPU resources. `EnvironmentPass` and `SkyboxPass` use the same public pass contract, allowing standalone execution and application replacements. M01-08 implements conversion/generation, validation and skybox services; see [environment-tools.md](environment-tools.md).

## Lifetimes and recording

All targets are single sample. Nonzero output/internal extents are resolved before initialization; a minimized presentation skips work. Existing `FrameContext::extent` describes output, while layer resources may use internal extent. `FrameContext::frameNumber` remains the presentation sequence. M01-05 adds `sceneFrameNumber` for successful scene submissions and `sceneDeltaSeconds` for active time between them; see [runtime-renderer.md](runtime-renderer.md).

Initialize components against one engine, attach a retained scene root, then update and record. Scene replacement resets scene-dependent resources. Resize, shader/pass replacement, GPU edits and cleanup require completion of outstanding users. Cleanup releases engine, scene and pass resources before engine teardown. GPU wrapper destruction alone is not a synchronization boundary. DefaultRenderLoopDelegate coordinates this through M01-05 lifecycle boundaries.

A `PassContext` and all its references are borrowed for the recording call. Its selected configuration must be the exact `LayerSelection::configuration` passed to dispatch. Images must remain alive until GPU completion. Slot order is significant. Recording implementations document image layouts and dependencies; the graph owns scheduling and supplies correct transitions. Inputs and outputs sharing an image require an explicitly supported read/write algorithm, not accidental aliasing.

## Compatibility checks

Implemented structural checks reject zero extents, invalid color/depth format categories, absent layers, extent mismatches, shader interface mismatches, differing slot counts/order/format/type/usage/domain, empty/duplicate semantics and incompatible image bindings. Bound images must be valid and belong to the frame engine's device.

Additional checks at GPU realization remain required: supported format/usage combinations, limits and feature availability (M01-02), actual usage/subresource/mipmap ranges (M01-03), shader modules and backend-specific binding/stage ABI (M01-04/M01-10), cubemap/IBL mip requirements (M01-08), graph producer/consumer dependencies and illegal aliasing (M01-10). An ABI string is a compatibility declaration, not shader reflection or proof of backend support. There is no silent conversion or unsupported-feature fallback in this contract layer; concrete renderers define their fallback policy.

## Shared high-level initialization modes

M01-06/M01-07 and subsequent shared-object integrations follow a simple three-state rule: uninitialized, production, draw. An explicit engine-specific initializer selects one mode and records its engine/device context. Reinitialization in a different mode requires synchronized cleanup first. Wrong-mode typed access throws an explicit error; it never initializes a second engine or casts between Drawable types. CPU-only serialization can remain uninitialized.

New `draw::Drawable` owns its CPU/GPU realizations. Shared `DrawableComponent`, DB and cache paths add typed overloads and accessors, preserving existing production calls through M08. No external ResourceManager or generalized compatibility layer is introduced. M09 removes the production mode, overloads and original rendering implementation after product acceptance. This step documents the rule without modifying those later-step objects.

## Project lead verification

Compile existing library/draw targets on the available Vulkan/Metal configurations and include `draw/Renderer.hpp`, `draw/pipeline/RenderLayer.hpp` and `draw/environment/all.hpp` from application code. No new executable or shader exists at this step. A custom layer can implement the abstract lifecycle, provide a matching contract and use `Renderer::recordLayer`; matching bindings reach its virtual record, incompatible contracts/bindings throw before that call. Runtime products, no-RT fallback and FSR remain later steps. No agent compilation/runtime checks were performed.
