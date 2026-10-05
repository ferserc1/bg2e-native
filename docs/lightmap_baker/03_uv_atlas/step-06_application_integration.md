# Step 06: Integrate UV2 generation and previews

## Scope

In model_edit expose Generate UV2 and UV1/UV2 preview for the active model; in bg2e_composer expose them for chosen Drawable nodes from the baker window. Use safe GPU reloads transparently to users. Complete lightmap_generator --generate-uv2 before GPU load or reload explicitly where needed; save generated .bg2 copies only in the output directory. Let both editors bake the newly generated atlas and keep all submesh AO materials on one texture.

## Acceptance and compile gate

Both editors and CLI compile; atlas generation, preview, bake and output work end to end. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Handoff

After this step, complete [prestep_07_documentation.md](prestep_07_documentation.md) for the next step (Document UV2 generation and previews).
