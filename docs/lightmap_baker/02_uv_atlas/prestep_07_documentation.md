# Handoff for Document UV2 generation and previews

Next implementation step: [step-07_documentation.md](step-07_documentation.md). Complete this file **after** finishing step 06; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in step-07_documentation.md.

## Next-step instructions

Update doc/api/geo, doc/api/render and doc/api/ui for the implemented atlas and editor preview. Cover MIT license, UV1 preservation, shared atlas across submeshes, CPU-only modifier and safe editor reload. The phase-3 CLI documents its own flag and outputs after implementation.

## Next-step acceptance gate

Documentation matches the completed behavior and links between namespaces. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Add `doc/api/geo/GenerateUv2AtlasModifier.md` and `UvAtlasValidator.md` with CPU-only behavior, xatlas MIT attribution, transactional failure, UV1 preservation and one-atlas/multiple-submesh semantics. Update geo index/reference.
- Add `doc/api/render/UvMapPreviewRenderer.md` and `doc/api/ui/UvMapPreview.md`; show both UV channels, image lifetime and the rule forbidding ImGui headers outside the engine UI implementation. Update namespace indexes.
- Update `doc/api/render/LightmapBaker.md` and `doc/lightmap_baking.md` with the integrated editor flow. Include the safe `MainLoop::safeUpdateScene` reload example. State that phase 3 will add headless CLI use of the same modifier.
- Check every code example against actual signatures and all Markdown links. CLI output cases are documented in phase 3.
