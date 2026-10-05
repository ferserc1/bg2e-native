# Step 06: Integrate UV2 generation and previews

## Scope

In model_edit expose Generate UV2 and UV1/UV2 preview for the active model; in bg2e_composer expose them for chosen Drawable nodes from the baker window. Use safe GPU reloads transparently to users. Complete lightmap_generator --generate-uv2 before GPU load or reload explicitly where needed; save generated .bg2 copies only in the output directory. Let both editors bake the newly generated atlas and keep all submesh AO materials on one texture.

## Acceptance and compile gate

Both editors and CLI compile; atlas generation, preview, bake and output work end to end. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- Add Generate UV2 to `ModelLightmapWindow` for the active model and to `SceneLightmapWindow` for selected Drawable nodes; pass the square bake resolution and padding to the modifier. Display `UvMapPreview` with UV1/UV2 toggle in both apps. Perform transparent safe reload through step 05, then allow a new bake with reset history. Handle modifier failure without changing the model or its AO assignment.
- In Composer, generate one atlas per selected Drawable, each covering **all** its submeshes; never pack multiple Drawables into one lightmap. The window list refreshes on scene changes and clears stale target references. In model_edit, retain existing names/group metadata and mark the document dirty on success.
- Complete `lightmap_generator --generate-uv2=true`: run the modifier before loading the target mesh onto GPU, then assemble/update the full scene TLAS; write a new .bg2 copy plus its image into the output directory. With `false`, keep the phase-2 images-only behavior and skip invalid UV2 with warnings. Input resources are never overwritten.
- Verify visual UV1 equality before/after, UV2 island layout, two-submesh shared texture, bake result in AO and CLI output matrix.

## End-to-end example

One two-submesh model produces one UV2 atlas, one PNG, and both submesh AO properties point to that same PNG.

## Handoff

After this step, complete [prestep_07_documentation.md](prestep_07_documentation.md) for the next step (Document UV2 generation and previews).
