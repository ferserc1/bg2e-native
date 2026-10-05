# Handoff for Integrate UV2 generation and previews

Next implementation step: [step-06_application_integration.md](step-06_application_integration.md). Complete this file **after** finishing step 05; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in step-06_application_integration.md.

## Next-step instructions

In model_edit expose Generate UV2 and UV1/UV2 preview for the active model; in bg2e_composer expose them for chosen Drawable nodes from the baker window. Use safe GPU reloads transparently to users. Complete lightmap_generator --generate-uv2 before GPU load or reload explicitly where needed; save generated .bg2 copies only in the output directory. Let both editors bake the newly generated atlas and keep all submesh AO materials on one texture.

## Next-step acceptance gate

Both editors and CLI compile; atlas generation, preview, bake and output work end to end. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.
