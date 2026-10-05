# Handoff for Generate one atlas across submeshes

Next implementation step: [step-02_atlas_modifier.md](step-02_atlas_modifier.md). Complete this file **after** finishing step 01; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in step-02_atlas_modifier.md.

## Next-step instructions

Add a geo modifier for the standard Mesh. Feed all submeshes into one xatlas packing operation; preserve each submesh's triangle range and material association. Use xatlas source-vertex cross-references to duplicate vertices at seams while copying position, normal, tangent and UV1 exactly; assign only UV2. Reject multi-atlas output, invalid triangles and unrepresentable results without partial mesh mutation. Expose resolution, padding and atlas diagnostics.

## Next-step acceptance gate

CPU fixtures show exactly one nonoverlapping UV2 atlas and unchanged UV1 values after seam splits. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.
