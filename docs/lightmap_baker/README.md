# Lightmap baker implementation plan

This plan is written in English as required by AGENTS.md. Work proceeds in order: [integrated baker](01_integrated_baker/overview.md), [standalone baker](02_standalone_baker/overview.md), then [UV atlas](03_uv_atlas/overview.md). Each step is intended as a separate reviewable change that leaves the project compilable. A step may initially be incomplete at runtime, but each phase aims to restore useful behavior as early as possible.

**Read the [API and behavioral contract](API_CONTRACT.md) before reviewing or implementing any step.** It fixes class names, ownership, example calls, pixel meaning, CLI options, UI behavior, and validation rules. Each step below links back to this contract and identifies its own concrete deliverables.

## Fixed decisions

- Use only the production `bg2e::render` ray tracing path, specifically `render::vulkan::rt::RayTracingScene`; `bg2e::gpu` migration is outside this plan.
- A scene-wide context owns and shares its own production TLAS resources across target bakers. In integrated mode the context owns one `RayTracingScene` per in-flight frame slot; these are independent of `FrameResources::rayTracingScene` and survive its `flushFrameData()`. In standalone mode the context owns its headless `RayTracingScene`. A per-Drawable baker owns its target, intermediate images, accumulation history and RGB CPU result. The two context types expose different scene lifecycles.
- The UV-space surface pass reuses `GBufferManager` for attachment ownership through an additive depthless, configurable-format profile. It keeps separate per-target/per-slot attachments; the existing camera G-buffer profile and deferred-renderer APIs remain intact.
- Validate target Drawable membership against the supplied scene root. The same lightmap texture is assigned to the existing AO property of every submesh material, using UV2. Do not add a material property or use emission.
- RTAO and RTGI are exclusive. RT shadows are an independent option. All bake layers use full target resolution. RGB8 is the normal output; configurable RGB32F CPU output is available. HDR file export is deferred.
- A command-line context scene and model/prefab targets are assembled into one scene and one TLAS. Standalone context alone drives component lifecycle. Integrated context uses an already running windowed or offscreen lifecycle.
- The CLI exports images in stb-supported PNG/JPEG/BMP/TGA formats. It writes .bg2 copies only when UV2 generation is requested, and never overwrites input resources. No new JSON prefab is emitted.
- xatlas is MIT licensed and generates UV2 on the CPU after the bake paths are available. UV1 remains unchanged. Editor reload is explicit and GPU-safe.

## Review and handoff convention

Each step has its own `step-XX_title.md`. Before starting a step, the implementing agent **must read both that step file and its corresponding `prestep_XX_description.md`**. For the first step of phases 2 and 3, the corresponding prestep is in the preceding phase's directory; all other presteps are beside their step files. The prestep may contain findings or constraints discovered while implementing the preceding step; its contents take precedence over the original plan where they record an approved correction.

After implementing and reviewing a step, the agent **must review the prestep file for the next step**. If the implementation revealed new dependencies, changed files or APIs, build results, runtime behavior, limitations, or cautions relevant to that next step, the agent **must update that next prestep file** before handing off. Record concrete findings and the exact next action; do not leave relevant discoveries only in a chat response. The prestep files initially supplied with this plan are templates, not claims that implementation has happened. The last documentation step of phases 1 and 2 hands off to the first step of the next phase. Phase 3 ends after documentation because no next step exists.

The implementation change that introduces apps/lightmap_generator or vendors xatlas requires CMake edits. AGENTS.md permits those edits only on an explicit implementation request; this planning task does not edit CMake. Implementers must receive that authorization before executing those steps. AGENTS.md also prohibits compiling without an explicit request; each implementation step's compile gate must be run when implementation is authorized.

## Out of scope

UI blocking/progress during expensive baking, HDR image export, renaming the AO material property, new material lightmap properties, changes to emission, and migration to `bg2e::gpu`.
