# Handoff for Document standalone baking

Next implementation step: [step-08_documentation.md](step-08_documentation.md). Complete this file **after** finishing step 07; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in step-08_documentation.md.

## Next-step instructions

Update doc/api/render and doc/api/app with standalone context lifecycle, CLI usage and both UV2 output branches. Add db image-format API docs and a technical note in doc/ if useful.

## Next-step acceptance gate

CLI help, documentation and implemented behavior agree. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Add `doc/api/render/StandaloneBakerContext.md` with explicit `initialize -> updateScene -> createBaker -> update -> readPixels -> cleanup` flow and no-window example. Update the render API index/reference.
- Document `ImageFormat` and helper functions in `doc/api/db/`, creating the namespace index if absent. Document CLI model/prefab flags, defaults, exit conditions and all four file output cases under `doc/api/app/`.
- Add a technical note in `doc/lightmap_baking.md` (or extend the integrated note) explaining full-scene assembly, shared TLAS, optional scene updates and component lifecycle. Confirm examples match actual compiled signatures and relative links resolve.
