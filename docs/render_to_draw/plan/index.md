# Render responsibilities reimplementation: milestone plan

**Plan baseline:** 2026-10-14, repository commit `12dac9eba55411267d0025b6bcaa31f4a99de3e7` and the user's conclusions following the [architecture assessment](../index.md).

This plan reimplements production rendering responsibilities over `bg2e::gpu`, reorganizes engine responsibilities, and eventually removes `bg2e::render`. It is not a namespace move or an implementation delivered by this document. No source/build files were changed and no build was run while preparing the plan.

The user's conclusions take precedence over recommendations in the earlier assessment. In particular, do not implement its external Drawable cache proposal, permanent compatibility layer, private-pass preference, or device-scoped ResourceManager redesign. [Architecture decisions](architecture-decisions.md) records the operative design; the older analysis remains source evidence and historical rationale.

## Milestones and working products

| ID | Milestone | Working product at completion | Main prerequisite |
|---|---|---|---|
| M01 | [Initial deferred renderer](milestone_01_deferred_renderer/summary.md) | `examples/draw/10_deferred_renderer`: Vulkan/Metal deferred scene, optional query soft shadows, resolution scaling and optional FSR, new runtime renderer delegate/settings. | Existing GPU/draw/application foundations. |
| M02 | [Manipulation](milestone_02_manipulation/summary.md) | `examples/draw/11_manipulation`: picking, editor manipulation and material modifications through `utils::MaterialModifier`. | M01. |
| M03 | [Complete ray tracing](milestone_03_ray_tracing/summary.md) | Both existing new examples gain supported RTAO/RTGI/reflections, histories and denoise; no new example. | M01–M02 and an explicit backend RT design checkpoint. |
| M04 | [Integrated lightmaps](milestone_04_lightmaps/summary.md) | `examples/draw/12_lightmaps`: generated UV2 and integrated baked maps, live GI disabled, progress/reset and correct material application. | M03. |
| M05 | [Offscreen applications](milestone_05_offscreen/summary.md) | `examples/draw/13_offscreen_application`: one output image to a CLI directory, configurable output/internal resolution and accumulated scene frames. | M01/M03; follows M04 in the agreed sequence. |
| M06 | [Model editor integration](milestone_06_model_edit/summary.md) | `apps/draw/model_edit` with the production application's useful behavior on draw, including assets, editing and baking. | M01–M05. |
| M07 | [Composer integration](milestone_07_bg2e_composer/summary.md) | `apps/draw/bg2e_composer` with scene composition, import, reflection/editor tools and baking. | M06. |
| M08 | [Lightmap generator integration](milestone_08_lightmap_generator/summary.md) | `apps/draw/lightmap_generator`: standalone bake contexts, batch/assembly/output workflow. | M04–M07. |
| M09 | [Final cleanup and promotion](milestone_09_cleanup/summary.md) | Render namespace and its entry points removed; draw is production, all supported executables use the new API. No previous application API support. | All earlier product gates accepted. |
| M10 | [Final documentation audit](milestone_10_documentation/summary.md) | All maintained documentation under `doc/` matches the final engine, products and extension points. | M09. |

Each directory contains `summary.md` and an ordered file per implementation/review/documentation step. Step IDs, rather than tentative filenames/class names, are the stable dependency references. M01–M09 close with a **code-versus-remaining-plan review followed by a separate cumulative documentation step**. Both are required. Maintained documentation covers only implementation delivered through that milestone; M10 audits the resulting complete set. See [documentation policy and preserved coverage](documentation-policy.md).

## Execution and verification rules

The [cross-chat execution protocol](execution-protocol.md) and [shared progress record](progress.md) govern step selection, handoff and acceptance. Read both before every implementation or correction. The human user is the project lead and owns compilation/runtime checks after each step. Their instruction to continue accepts the preceding delivered step; no repeated confirmation is required.

All implementation and review work must also satisfy the repository source-style/license rules in `AGENTS.md`, made explicit for this plan in [execution-protocol.md](execution-protocol.md): full GPL headers on repository-owned C/C++ and shader files, blank lines between `.cpp` function definitions, and purpose-grouped class/struct methods separated by blank lines. Review steps check these rules explicitly.

For a new chat, referencing **this index and progress.md is sufficient**. The agent must open the actual step linked in progress.md's **Next step to implement** field without asking the project lead to identify it. Every step delivery updates that link, its next action and any acceptance condition; a reported failure points back to correction work instead of advancing.

1. Read the relevant repository instructions, execution protocol/progress, current code, the milestone summary, its selected step and the latest actual review report before implementing. Select work using the protocol, not another chat's memory. Implement one step per request and record its handoff.
2. Preserve production behavior as a readable reference through M08. Add overloaded engine paths to shared high-level objects where useful. Do not introduce a generalized compatibility architecture solely to avoid simple dependencies.
3. A milestone is complete when its required product/review work is implemented and accepted by the project lead. Static inspection does not establish runtime behavior. Lead continuation is valid acceptance under the agreed convention; record its basis without fabricating test/backend details or demanding logs.
4. Delegate compilation, shader compilation, runtime tests and build-producing scripts to the project lead after every step. The agent performs relevant nonexecuting static checks and runs builds/tests only when explicitly instructed. Deliver checks/expected behavior, update progress to `awaiting_project_lead`, and stop. A failure report requires correction; continuation accepts the delivered correction and authorizes the next step.
5. CMake/example/application/shader packaging changes are identified as future work required by the products. No CMake changes are made now. At execution, follow the explicit authorization and repository policy then in force; do not silently omit runnable targets because sources are globbed into the library.
6. Metal and Vulkan are target backends. Include applicable raster/no-RT and optional-feature cases in the checklist delegated to the lead. Record reported coverage accurately; unspecified coverage does not block acceptance under their continuation convention and does not justify claiming all backends were tested or removing a fallback.
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
| Review and revise remaining plan after each milestone | [Review protocol](review-protocol.md) | Every technical review, then final cumulative documentation | M10 audits the complete accumulated documentation. |

## Reading order and current status

Read [execution-protocol.md](execution-protocol.md) and [progress.md](progress.md) first, then [architecture-decisions.md](architecture-decisions.md), [review-protocol.md](review-protocol.md) and the selected milestone/step. [Source anchors](source-anchors.md) identifies current APIs/documents to inspect. Execution status belongs in the shared progress record, rather than in generic planning text or chat history.

[Requirement coverage](requirements-map.md) maps the user's conclusions to concrete step files. The plan currently contains ten milestone summaries and 89 implementation/review/documentation step documents. Filenames identify proposed outputs; only source links point to existing files.

The incomplete sentence in the requested M03 deliverable is interpreted as verification of the existing `deferred_renderer` and `manipulation` examples. No extra example is introduced in that milestone. New lightmap/offscreen example names are the proposed concrete paths listed above; preserve the user's explicitly specified paths elsewhere.

M01-12 implementation contract: [live settings catalog, codec and facade](milestone_01_deferred_renderer/reflected-settings.md). See progress for its validation state and the M01-13 successor.

M01 review delivery: [actual contracts and M02–M10 impact](milestone_01_deferred_renderer/review.md). See progress.md for correction/acceptance state. M01-16 review now hands off to [M01-17 documentation](milestone_01_deferred_renderer/step_17_documentation.md); M02-01 follows only after that documentation closure is accepted.

## Distributed documentation closure

Each milestone documentation step updates the canonical maintained pages for the cumulative implementation, not future planned APIs. M09 finishes production-status/retirement and whole-document reconciliation; M10 audits that delivered set. The [coverage/ownership policy](documentation-policy.md) preserves all original final documentation outcomes.


## M04 technical review handoff

The [implemented M04 review](milestone_04_lightmaps/review.md) records integrated
baking, explicit RGB UV2 material adoption, CPU UV diagnostics and corrected
paired bake shader deployment, with validation provenance and pending impacts.
That review was followed by M04-08 cumulative documentation. Use
[progress](progress.md) for the active acceptance/next-step pointer.


## M05 review and documentation handoff

[Actual M05 contracts](milestone_05_offscreen/review.md) and
[cumulative documentation coverage](milestone_05_offscreen/documentation.md) record
windowless draw ownership, exact camera samples, settings precedence and completed
PNG export. M05-07 documentation is delivered awaiting lead acceptance; M06-01 is
the next required step after that closure. Production render coexistence and
independent UV-bake/batch ownership remain explicit.
