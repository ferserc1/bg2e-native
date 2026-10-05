# Step 07: Document UV2 generation and previews

## Scope

Update doc/api/geo, doc/api/render, doc/api/ui and relevant doc/api/app/db pages. Cover MIT provenance, UV1 preservation, shared atlas across submeshes, CPU-only modifier, safe editor reload, CLI flag and UV preview. Add a technical overview in doc/ if it clarifies the flow.

## Acceptance and compile gate

Documentation matches the completed behavior and links between namespaces. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- Add `doc/api/geo/GenerateUv2AtlasModifier.md` and `UvAtlasValidator.md` with CPU-only behavior, xatlas MIT attribution, transactional failure, UV1 preservation and one-atlas/multiple-submesh semantics. Update geo index/reference.
- Add `doc/api/render/UvMapPreviewRenderer.md` and `doc/api/ui/UvMapPreview.md`; show both UV channels, image lifetime and the rule forbidding ImGui headers outside the engine UI implementation. Update namespace indexes.
- Update `doc/api/app/` CLI usage and `doc/api/render/LightmapBaker.md` to remove the phase-2 UV2 flag limitation, and expand `doc/lightmap_baking.md` with the full flow. Include the safe `MainLoop::safeUpdateScene` reload example.
- Check every code example against actual signatures, all Markdown links, and the four CLI output cases (model/prefab × generate-uv2 true/false).

## Final review

The permanent documentation must state that the GUI invokes reload after safe GPU idle, while the `geo` modifier never performs GPU work.

## Handoff

This is the final step; no next-step handoff file is needed.
