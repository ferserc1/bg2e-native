# Step 08: Document standalone baking

## Scope

Update doc/api/render and doc/api/app with standalone context lifecycle, CLI usage and output behavior. Add db image-format API docs and a technical note in doc/ if useful. State the initial UV2-regeneration flag limitation until phase 3 completes.

## Acceptance and compile gate

CLI help, documentation and implemented behavior agree. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- Add `doc/api/render/StandaloneBakerContext.md` with explicit `initialize -> updateScene -> createBaker -> update -> readPixels -> cleanup` flow and no-window example. Update the render API index/reference.
- Document `ImageFormat` and helper functions in `doc/api/db/`, creating the namespace index if absent. Document CLI model/prefab flags, defaults, exit conditions, file output matrix and lack of regenerated UV2 until phase 3 under `doc/api/app/`.
- Add a technical note in `doc/lightmap_baking.md` (or extend the integrated note) explaining full-scene assembly, shared TLAS, optional scene updates and component lifecycle. Confirm examples match actual compiled signatures and relative links resolve.

## Documentation example

Show two target bakers sharing one `updateScene` and one TLAS without a render loop.

## Handoff

After this step, complete [prestep_01_xatlas_dependency.md](prestep_01_xatlas_dependency.md) for the next step (Introduce xatlas under MIT).
