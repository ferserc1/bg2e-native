# Render to draw: architecture and migration assessment

**Review snapshot:** 2026-10-13, commit `12dac9eba55411267d0025b6bcaa31f4a99de3e7`.

**Implementation planning update (2026-10-14):** the user's subsequent conclusions and the [ten-milestone implementation plan](plan/index.md) supersede conflicting architectural recommendations below. This assessment remains the original source inventory; consult [the operative decisions](plan/architecture-decisions.md) before implementation.

This is a source-based assessment for architectural review before an implementation plan is approved. It describes existing behavior, suggests alternatives, and identifies dependencies and acceptance criteria. Proposed names and interfaces are illustrative; they are not implemented contracts. No engine code or CMake files were changed, and no compilation or runtime verification was performed.

## Executive overview

The migration is substantially larger than porting `RendererDeferred`. Production `render` combines a Vulkan resource framework, engine execution, scene extraction, materials and environment resources, deferred effects, and graphics tools. `scene::Drawable` combines CPU geometry and editable attributes with production GPU meshes, materials, BLAS objects, and Vulkan drawing callbacks. Component deserialization also carries `render::Engine` across most of the scene graph. Those boundaries determine the migration effort more than the namespace rename.

The experimental framework already has useful foundations. `draw::Engine`, `FrameContext`, `RenderLoop`, and `RenderLoopDelegate` support the managed application loop and retained scene/UI composition. `gpu` supplies Vulkan and Metal resources, indexed meshes, compute, cubemaps, acceleration structures, and ray tracing pipelines. Experimental examples demonstrate many of these primitives. They do **not** establish parity with production materials, scene loading, deferred layers, selection, FSR, or baking.

The proposed ownership boundary follows the additional review criteria:

- **`gpu` remains equivalent in scope to a low-level graphics API.** Multiple render attachments, image formats, pipeline state, resource arrays, and non-opaque ray tracing geometry are legitimate GPU primitives. G-buffers, scene bindings, lighting policies, material caches, effect histories, and layer composition belong in `draw`.
- **`draw` owns high-level graphics resources and the renderer framework.** It should contain an abstract `Renderer` and one concrete deferred renderer. No forward renderer is proposed.
- **`scene` retains scene data and graph behavior.** A narrow `Drawable` connection between meshes and GPU buffers is acceptable. `scene::vk` and renderer-specific visitors should leave the scene namespace. Deserialization should evolve toward a resource-loading context instead of a production engine parameter.
- **Production and experimental APIs can coexist during migration.** Preserve production objects initially, add a narrow bridge that reads their CPU data, and develop clean experimental resources independently. Later, a neutral geometry/data contract can support both. Simultaneous execution, binary compatibility, and zero-copy GPU sharing are separate requirements and are not automatically preserved.

Three particularly consequential findings are:

1. The production G-buffer currently has **six color attachments plus depth**, including the baked RGB light multiplier. Historic deferred plans describe older layouts. `gpu` currently exposes a single color attachment and fixed graphics state, so a primitive rendering-contract extension precedes a faithful deferred port.
2. RT parity includes **alpha-cutout intersections and per-instance material/texture lookup**, not merely BLAS/TLAS creation. Production GI and reflections use an any-hit shader; experimental Vulkan geometry is currently opaque and the public RT pipeline has no any-hit stage. Resource array assignment is also insufficient for production's object tables.
3. The no-RT baseline is already meaningful: raster PBR, IBL, transparency/refraction, baked lighting, and tools remain usable. Production does not provide equivalent software RT, SSAO, SSR, or shadow-map fallbacks. Adding those would be a separate expansion, not a prerequisite for preserving current behavior.

## Reading guide

| Document | Purpose |
|---|---|
| [01 — Production inventory](01-production-inventory.md) | Class families, mixed responsibilities, and what should move, split, stay, or be replaced. |
| [02 — Dependency map](02-dependency-map.md) | Dependencies in both directions, cycles, public API contamination, and affected consumers. |
| [03 — Scene boundary](03-scene-boundary.md) | Detailed `Drawable`, bindings, visitors, deserialization, clone, and resource-ownership analysis. |
| [04 — Drawable alternatives](04-drawable-alternatives.md) | Four refactoring options, compatibility costs, and a recommended transition. |
| [05 — Target draw architecture](05-target-draw-architecture.md) | Proposed engine objects, abstract renderer contract, extraction, layers, settings, and ownership. |
| [06 — GPU primitives and readiness](06-gpu-primitives-and-readiness.md) | Existing experimental facilities and low-level gaps, explicitly separated from `draw` responsibilities. |
| [07 — Deferred pipeline and parity](07-deferred-pipeline-and-parity.md) | Current pass order, G-buffer schema, transparency, complete feature matrix, and acceptance conditions. |
| [08 — Ray tracing and hardware fallback](08-ray-tracing-and-fallback.md) | RTAO, RTGI, reflections, soft shadows, alpha cutouts, histories, and no-RT behavior. |
| [09 — Materials, IBL, and shaders](09-materials-environment-and-shaders.md) | Texture/material resources, CPU/GPU separation, environment processing, shader interfaces, and backend differences. |
| [10 — Selection, UI, and graphics tools](10-selection-ui-and-tools.md) | Picking, highlight, gizmos, settings, UV previews, and viewpoint analysis. |
| [11 — Offscreen and baking](11-offscreen-and-baking.md) | Headless execution, export, integrated/standalone baking, and hidden scope beyond the viewport. |
| [12 — Coexistence, magnitude, and review decisions](12-coexistence-magnitude-and-decisions.md) | Compatibility levels, retirement path, work packages, risks, sequencing constraints, and decisions needed before planning. |
| [13 — Source inventory and dependency evidence](13-source-evidence.md) | Exhaustive scoped file inventory, direct include edges, consumer references, and measurement limitations. |

For a first review, read this overview, documents 03–06, and document 12. For functional completeness, read 07–11. Document 13 is a traceability appendix rather than the recommended entry point.

## Magnitude

The scoped production tree contains **93 header files and 85 implementation/private-source files**, approximately **39,498 physical lines** including comments and license text. The experimental `draw` tree contains six headers and two implementations, approximately 1,014 lines. These are surface measurements, not port estimates: Vulkan wrappers have replacements in `gpu`, forward rendering is excluded, and much high-level code requires restructuring rather than copying.

A textual scan outside `render` identifies direct production-header includes in 24 engine headers, 17 engine implementation files, seven application files, and one example file. Including qualified `render::` references raises those counts to 45, 45, 19, and 27. Transitive dependencies through `all.hpp` and `scene::Drawable` make the practical impact larger; the scan is not an exhaustive semantic call graph.

The highest-risk work is the scene/resource boundary, deferred GPU primitives, RT material addressing and cutout parity, temporal synchronization, FSR interoperability, and headless/baking integration. Renaming headers or duplicating the old hierarchy would leave most of those problems intact.

## Review recommendation

Adopt an additive transition: independent `draw` resources and renderer; a quarantined CPU-data bridge for current scene objects; a future neutral `Drawable` geometry/buffer contract; renderer-specific scene extraction and bindings in `draw`; old GPU-facing accessors confined to a removable production compatibility layer. Prefer this over either placing the new renderer inside `scene` or turning `gpu` into a scene/material framework.

If preserving every public production symbol requires maintaining the existing mixed `Drawable` for a while, accept that as **temporary legacy debt**, not as the target scene API. The user explicitly allows compatibility compromises in exchange for a clean experimental API. Document 04 compares the alternatives and explains when a deliberate break is simpler.

## Evidence and limits

Statements labeled **Observed** describe checked repository declarations or implementations. **Proposed** statements are architectural suggestions. Risk and effort assessments are engineering judgments, not measured runtime results. Existing documentation and examples provide context, but current implementation takes precedence where they disagree. The referenced `skills.md` architecture file was not found in the repository; no engine internals were modified. This assessment does not promise device-specific support, frame-rate targets, identical images across backends, or ABI compatibility.
