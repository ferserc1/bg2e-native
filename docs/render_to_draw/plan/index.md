# Render responsibilities reimplementation: milestone plan

**Plan baseline:** 2026-10-14, repository commit `12dac9eba55411267d0025b6bcaa31f4a99de3e7` and the user's conclusions following the [architecture assessment](../index.md).

This plan reimplements production rendering responsibilities over `bg2e::gpu`, reorganizes engine responsibilities, and eventually removes `bg2e::render`. It is not a namespace move or an implementation delivered by this document. No source/build files were changed and no build was run while preparing the plan.

The user's conclusions take precedence over recommendations in the earlier assessment. In particular, do not implement its external Drawable cache proposal, permanent compatibility layer, private-pass preference, or device-scoped ResourceManager redesign. [Architecture decisions](architecture-decisions.md) records the operative design; the older analysis remains source evidence and historical rationale.

## Milestones and working products

| ID | Milestone | Working product at completion | Main prerequisite |
|---|---|---|---|
| M01 | [Initial deferred renderer](milestone_01_deferred_renderer/summary.md) | `examples/draw/deferred_renderer`: Vulkan/Metal deferred scene, optional query soft shadows, resolution scaling and optional FSR, new runtime renderer delegate/settings. | Existing GPU/draw/application foundations. |
| M02 | [Manipulation](milestone_02_manipulation/summary.md) | `examples/draw/manipulation`: picking, editor manipulation and material modifications through `utils::MaterialModifier`. | M01. |
| M03 | [Complete ray tracing](milestone_03_ray_tracing/summary.md) | Both existing new examples gain supported RTAO/RTGI/reflections, histories and denoise; no new example. | M01–M02 and an explicit backend RT design checkpoint. |
| M04 | [Integrated lightmaps](milestone_04_lightmaps/summary.md) | `examples/draw/lightmaps`: generated UV2 and integrated baked maps, live GI disabled, progress/reset and correct material application. | M03. |
| M05 | [Offscreen applications](milestone_05_offscreen/summary.md) | `examples/draw/offscreen_application`: one output image to a CLI directory, configurable output/internal resolution and accumulated scene frames. | M01/M03; follows M04 in the agreed sequence. |
| M06 | [Model editor integration](milestone_06_model_edit/summary.md) | `apps/draw/model_edit` with the production application's useful behavior on draw, including assets, editing and baking. | M01–M05. |
| M07 | [Composer integration](milestone_07_bg2e_composer/summary.md) | `apps/draw/bg2e_composer` with scene composition, import, reflection/editor tools and baking. | M06. |
| M08 | [Lightmap generator integration](milestone_08_lightmap_generator/summary.md) | `apps/draw/lightmap_generator`: standalone bake contexts, batch/assembly/output workflow. | M04–M07. |
| M09 | [Final cleanup and promotion](milestone_09_cleanup/summary.md) | Render namespace and its entry points removed; draw is production, all supported executables use the new API. No previous application API support. | All earlier product gates accepted. |
| M10 | [Documentation](milestone_10_documentation/summary.md) | All maintained documentation under `doc/` matches the final engine, products and extension points. | M09. |

Each directory contains `summary.md` and an ordered file per implementation/review step. Step IDs, rather than tentative filenames/class names, are the stable dependency references. Each milestone ends with a **code-versus-remaining-plan review**; its report and actual updates to pending milestones are part of completion, not optional notes.

## Execution and verification rules

1. Read the relevant repository instructions, current code, the milestone summary, its steps and the latest accepted review report before implementing. The plan is a baseline, not permission to ignore later decisions.
2. Preserve production behavior as a readable reference through M08. Add overloaded engine paths to shared high-level objects where useful. Do not introduce a generalized compatibility architecture solely to avoid simple dependencies.
3. A milestone is complete only when its working product and exit checks are supported by evidence. Compilation alone does not establish graphics parity; static review alone does not establish a runnable product.
4. Respect the repository's explicit build restriction. Build/runtime checks in this plan are **future acceptance checks**, executed by the user or by an agent only when the user explicitly requests compilation. Record `not run`/`not available` accurately and do not claim an untested gate has passed. This documentation request does not authorize builds.
5. CMake/example/application/shader packaging changes are identified as future work required by the products. No CMake changes are made now. At execution, follow the explicit authorization and repository policy then in force; do not silently omit runnable targets because sources are globbed into the library.
6. Metal and Vulkan are target backends. Test the raster/no-RT path independently of optional RT/SDK support. Missing test hardware is a recorded verification gap, not proof of success or justification to remove the fallback.
7. Public renderer/pass configuration supports application-owned compatible replacements. No new private implementation class without a concrete necessity documented in review. Ordinary private members/functions are not prohibited.
8. No MSAA, forward renderer, ResourceManager, native Metal upscaler or DLSS implementation is included. Extension points for future upscalers are included. Advanced RT choices are deferred to M03, not silently settled by M01.

## Cross-cutting checkpoints

| Requirement | First contract | First working integration | Later completion |
|---|---|---|---|
| Public namespaces/configurable pass objects | M01-01 | M01-08/M01-10 | M03/M04 and M10. |
| Primitive MRT/state/capabilities/mips | M01-02/M01-03 | M01-10 | M03/M04/M05 additional formats/readback. |
| Separate shader folders containing both languages | M01-04 | M01-15 | M03/M04, cleanup M09. |
| Runtime renderer ownership/switching | M01-05 | M01-15 | Settings/extension checks through M08. |
| Draw Drawable and dual DrawableComponent | M01-06 | M01-15 | MaterialModifier M02; legacy removal M09. |
| Two DB levels, with engine-aware load/save | M01-07 | M01-15 minimal loader | Full import/save M06/M07/M08. |
| Data bindings and render visitors leave scene | M01-09 | M01-10 | Legacy implementation removed M09. |
| Stateless reflected preferences/config file | M01-12 | M01-14/M01-15 | Headless M05, apps M06–M08. |
| Extensible upscaling, default resolution reduction | M01-13 | M01-15 | Native Metal/DLSS remain future extensions. |
| Manipulation remains high-level | M02-01 | M02-07 | Applications M06/M07. |
| Generic BlueNoise in utils | M03-03 | M03-04 | Baking M04. |
| New viewpoint analysis in utils | M02-06 | M02 tool checks | App UI integration M06/M07. |
| Bake namespace, including preview/settings/pixels/batch | M04-01 | M04-06 | Standalone/batch M08. |
| OffscreenApplication production/draw overloads | M05-01 | M05-04 | Canonical draw-only path M09. |
| No Vulkan implementation outside GPU in new code | M01-01/M01-13 | Every product | Repository-wide final check M09. |
| Review and revise remaining plan after each milestone | [Review protocol](review-protocol.md) | Every final step | M10 final actual-code documentation review. |

## Reading order and current status

Read [architecture-decisions.md](architecture-decisions.md), [review-protocol.md](review-protocol.md), then M01. [Source anchors](source-anchors.md) identifies current APIs/documents that implementers should inspect. All milestones are **planned, not implemented**. No implementation report is fabricated here.

[Requirement coverage](requirements-map.md) maps the user's conclusions to concrete step files. The plan currently contains ten milestone summaries and eighty implementation/review step documents. Filenames identify proposed outputs; only source links point to existing files.

The incomplete sentence in the requested M03 deliverable is interpreted as verification of the existing `deferred_renderer` and `manipulation` examples. No extra example is introduced in that milestone. New lightmap/offscreen example names are the proposed concrete paths listed above; preserve the user's explicitly specified paths elsewhere.
