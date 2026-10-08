# Handoff for Document standalone baking

Next implementation step: [step-08_documentation.md](step-08_documentation.md). This handoff records step 07 implementation findings; project-lead compile and CLI fixture verification remain pending.

- Changed files and relevant API decisions: Added `apps/lightmap_generator/src/main.cpp`, `CommandLine.cpp` and `CommandLine.hpp`, plus the `apps/lightmap_generator/CMakeLists.txt` target and its entry in `apps/CMakeLists.txt`. The entry point parses/validates arguments and paths before `Engine::init()`, assembles model or prefab input in CPU-only target mode, invokes `render::StandaloneBakeBatch`, and reports skipped-target warnings, progress, cancellation and baked/skipped totals. The CLI defaults are PNG, 512 resolution, 16 frames, 8 samples per pixel and RTGI, matching `LightmapSettings`; UV2 generation defaults false. Mode-specific GI flags are rejected in RTAO mode.
- Changed loading/output workflow: The CLI calls the assembler's `loadTargetGpuResources=false` overload for both subcommands. The batch validates or generates target UV2 on CPU, loads target GPU/BLAS resources before its single `updateScene()` call, and uses `db::LightmapOutputWriter` for image-only or image-plus-.bg2 outputs. The app target uses `bundle_app()` to retain platform packaging, runtime resources and the standard output location.
- Build command, platform and result: Not run by the agent; repository instructions reserve compilation for the project lead.
- Runtime/fixture evidence: Not run. Project-lead verification should cover `--help` and invalid/duplicate/irrelevant arguments before Vulkan initialization, model and prefab runs with existing UV2, both UV2-generation branches, skipped-target warnings, output preservation and naming, and cancellation/summary exit behavior.
- Remaining limitations or regressions: The CLI and its build target are not yet compiled or exercised. The Engine and its standalone context/batch resources must be cleaned in lifetime order; the entry point lets the batch and assembly leave scope before `Engine::cleanup()`.
- Resources, ownership and synchronization cautions for the next agent: `StandaloneBakeBatch::run` requires an explicit `Options` argument. Its progress callback is continuation-valued (`false` cancels before the next sample); its warning callback receives each skipped identity and reason once. The CPU-only assembler overloads are `assembleModel(context, model, std::nullopt, false)` and `assemblePrefab(context, prefab, std::nullopt, false)`. Keep all load/bake/output logic out of `main.cpp`; the application is a thin integration layer.
- Exact next action: Obtain project-lead compile and CLI model/prefab branch verification for standalone step 07; after it passes, implement `step-08_documentation.md`.

## Next-step instructions

Update doc/api/render and doc/api/app with standalone context lifecycle, CLI usage and both UV2 output branches. Add db image-format API docs and a technical note in doc/ if useful.

## Next-step acceptance gate

CLI help, documentation and implemented behavior agree. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Add `doc/api/render/StandaloneBakerContext.md` with explicit `initialize -> updateScene -> createBaker -> update -> readPixels -> cleanup` flow and no-window example. Update the render API index/reference.
- Document `ImageFormat` and helper functions in `doc/api/db/`, creating the namespace index if absent. Document CLI model/prefab flags, defaults, exit conditions and all four file output cases under `doc/api/app/`.
- Add a technical note in `doc/lightmap_baking.md` (or extend the integrated note) explaining full-scene assembly, shared TLAS, optional scene updates and component lifecycle. Confirm examples match actual compiled signatures and relative links resolve.
