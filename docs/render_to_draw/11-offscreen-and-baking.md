# Offscreen rendering and lightmap baking

[Overview](index.md) · [RT resources](08-ray-tracing-and-fallback.md) · [Magnitude](12-coexistence-magnitude-and-decisions.md)

## 1. Offscreen is not one feature

Distinguish three existing/useful paths:

| Path | Purpose | Target integration |
|---|---|---|
| Render to a texture within a windowed application | Intermediate graphics/compute output. | Already demonstrated experimentally; reusable draw resource/pass objects needed. |
| Headless camera scene rendering | CLI/batch image output without window/UI. | New draw engine/execution entry point using GPU OffscreenSurface. |
| UV-space lightmap baking | Geometry-atlas AO/GI accumulation and export. | Independent optional draw baking subsystem, not camera renderer output. |

The existence of the first path does not imply the other two are complete.

## 2. Production headless execution

[`render::Engine`](../../lib/include/bg2e/render/Engine.hpp) has windowed and windowless initialization. [`app::OffscreenApplication`](../../lib/include/bg2e/app/OffscreenApplication.hpp) owns the production engine/color/depth resources and exposes native command/frame/image contracts to its delegate. [`DefaultOffscreenApplicationDelegate`](../../lib/include/bg2e/render/DefaultOffscreenApplicationDelegate.hpp) adapts a renderer to that lifecycle.

The two traditional CLI examples cover low-level/offscreen and scene-rendered output. Deferred renderer receives an offscreen flag and excludes gizmo overlays. General camera rendering does not inherently require RT support.

**Observed experimental gap:** `draw::Engine::init` rejects a null SDL window and owns `gpu::WindowSurface`; `draw::RenderLoop` is window/presentation-oriented. GPU has OffscreenSurface and examples for windowless context/triangle rendering. The missing part is high-level draw execution/configuration, not another low-level offscreen subsystem.

## 3. Suggested headless architecture

Expose a windowless draw context/configuration with output extent/format and selected backend. Reuse resource services and the same virtual/deferred renderer contract. Keep frame acquire/record/submit/completion ownership consistent with window execution, but avoid requiring UI, input, selection or a retained window target.

An application-facing offscreen adapter can provide synchronous render-and-export convenience and batched frame execution. Scene extraction and resource realization should function without booting production. The adapter controls camera choice, convergence/sample counts, output naming and file format; the renderer records graphics work.

Design questions include whether Engine holds a generic Surface, whether multiple targets share one Device, and whether a standalone render session wraps an existing device. Prefer explicit device ownership over silently reusing the Factory shared Instance in multiple simultaneously initialized engines. The current draw initialization intentionally checks exclusive initialized instance use.

## 4. Output and completion

Headless output must define:

- Final extent and internal scale independently.
- LDR/HDR format and color conversion/orientation.
- Required completion before readback or file export.
- Scene-render versus presentation/batch frame counters.
- Accumulation/warm-up policy for temporal effects and FSR.
- Whether optional unsupported effects cause fallback or an explicit job error.

GPU provides primitive readback/completion. Draw supplies conversion/processing when needed. DB writes image files. Standard rendering should remain available on no-RT hardware; a job requesting mandatory RT baking can fail cleanly without redefining the basic device as unsupported.

## 5. Existing baking API and private passes

Sources: [`LightmapBaker.hpp`](../../lib/include/bg2e/render/LightmapBaker.hpp), [`IntegratedBakerContext.hpp`](../../lib/include/bg2e/render/IntegratedBakerContext.hpp), [`StandaloneBakerContext.hpp`](../../lib/include/bg2e/render/StandaloneBakerContext.hpp), private implementation files under [`render`](../../lib/src/bg2e/render/).

| Object | Current responsibility |
|---|---|
| `BakerContext` | Engine and root references, shared lifetime. |
| `LightmapBaker` | Target validation, settings, progress, UV surfaces, per-slot AO/GI/result images, accumulation and readback. |
| `IntegratedBakerContext` | Frame preparation, isolated bake TLAS/descriptor allocators per slot, captured scene bindings and renderer environment access. |
| `IntegratedLightmapBaker` | Record one sample into a prepared matching production frame; forbid duplicate/invalid updates. |
| `StandaloneBakerContext` | Headless scene initialization/update, light/environment capture, tracing preparation and generation tracking. |
| `StandaloneLightmapBaker` | Synchronous sample execution tied to prepared scene generation. |
| `UvSurfacePass` | Render UV-space geometry into a depthless attribute/identifier target. |
| `UvTemporalAccumulator` | Accumulate samples and dilate atlas edges. |
| `LightmapCompositionPass` | Combine AO/GI into RGB indirect-light multipliers with valid-mask semantics. |
| `LightmapBakeExecutor` | Coordinate pass execution/resource snapshots. |
| `StandaloneBakeSceneAssembler` | Load context/model/prefab and select bake targets, optionally generate UV2. |
| `StandaloneBakeBatch` | Execute configured batches and output handling. |

This is already a subsystem. Porting only the viewport RTGI pass omits its UV-space inputs, resource isolation, accumulated output and state machine.

## 6. UV surface versus camera G-buffer

Baking reuses GBufferManager with a separate, depthless configuration. Its four outputs are RGBA32F world-space position, RGBA16F signed world normal, R32_UINT zero-based submesh/material identity, and R32_UINT valid-texel mask. Production bake passes validate these profiles and corresponding AO/GI output formats. This is not the six-channel camera G-buffer schema.

The new `draw` render-target owner should support named/configured profiles without teaching GPU what UV baking is. Reuse primitive MRT/integer attachments and graphics state. `geo::GenerateUv2AtlasModifier` and its xatlas dependency remain geometry preprocessing; only the graphics baking stage belongs in draw.

## 7. Integrated execution invariants

Production IntegratedBakerContext verifies that command/frame resources correspond to the engine's current frame, has its own prepared-frame identity, and maintains separate bake TLAS/descriptor allocators. It captures scene/material/light bindings and borrows active renderer EnvironmentResources for RTGI. Updating a baker twice in the same frame or after completed accumulation is rejected.

Preserve these invariants with an experimental bake context referencing a protected frame slot, command ownership and scene/resource generation. A new API should not require application code to manufacture Vulkan frame structs. Share an immutable environment resource handle with clear lifetime rather than reach into a concrete renderer for arbitrary internals.

Geometry, textures, scene targets and resources must remain coherent for the bake duration. Live edits can cancel/reset the bake or create a new snapshot; either is an explicit policy. Sharing the raster TLAS without defining snapshot/ownership differences risks contamination by runtime gizmos or changed filtering.

## 8. Standalone execution invariants

StandaloneBakerContext has created/initialized/scene-ready/cleaned states, explicit scene updates and a generation counter. Existing bakers become invalid after a new scene preparation. Synchronous work builds appropriate tracing resources and captures environment/lights separately from a viewport renderer.

Retain this separation. A CLI bake should not instantiate a deferred camera renderer solely to obtain IBL or material tables. Draw environment/tracing services must be independently usable. Multiple targets can share a prepared context with explicit target identity and output preflight.

## 9. Baked-light semantics and export

Lightmap settings cover RTAO/RTGI, resolution, sample/accumulation controls, ray distance/bias, GI bounces, exposure, pixel format and dilation. Pixels are represented as byte or float RGB data. Result textures represent an indirect-light multiplier, not simply raw camera-space GI color.

Preserve:

- Valid-mask handling and neutral white texels where appropriate.
- AO/GI composition and exposure semantics.
- UV set/material metadata used to apply the baked map.
- Edge dilation applied consistently to sampled output and CPU export.
- RGB8 and RGB32F readback/output without implicit LDR clipping.
- Target/submesh validation and mesh index range checks.
- Accumulation reset/progress and generation invalidation.

[`db::LightmapOutputWriter`](../../lib/include/bg2e/db/LightmapOutputWriter.hpp) already owns output preflight, input protection, optional model copies and atomic replacement behavior. Its dependence on `render::LightmapPixels` should become a neutral pixel-data contract. File-writing policy should not migrate into the draw renderer.

## 10. Suggested extraction boundary

Place reusable baking graphics/resources in an optional `draw::baking` family. Keep geometry atlas generation in geo, CPU resource loading/output in DB, and command-line batch configuration/orchestration in apps or an application-facing service. Existing production wrappers can continue to expose the old public API during coexistence.

A shared tracing material/light/environment service benefits both viewport and baking, but AO/GI pass inputs should remain distinct: camera depth reconstruction and UV surface reconstruction are different contracts. Do not force UV passes through a synthetic camera view to avoid designing an input profile.

## 11. Scope and risk

Headless camera rendering is a medium-to-large integration task even after primitive GPU offscreen support. Full baking is another large work package because it adds shader profiles, float/ID readback, lifecycle isolation, geometry preprocessing integration and output metadata.

If the first implementation milestone excludes baking, state that it is an interim milestone rather than full `render` feature parity. The user asked for all current capabilities except the forward renderer, so baking/tools must remain visible in the total assessment.
