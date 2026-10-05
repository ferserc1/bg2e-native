# Step 02: Generate one atlas across submeshes

## Scope

Add a geo modifier for the standard Mesh. Feed all submeshes into one xatlas packing operation; preserve each submesh's triangle range and material association. Use xatlas source-vertex cross-references to duplicate vertices at seams while copying position, normal, tangent and UV1 exactly; assign only UV2. Reject multi-atlas output, invalid triangles and unrepresentable results without partial mesh mutation. Expose resolution, padding and atlas diagnostics.

## Acceptance and compile gate

CPU fixtures show exactly one nonoverlapping UV2 atlas and unchanged UV1 values after seam splits. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Handoff

After this step, complete [prestep_03_uv_validation.md](prestep_03_uv_validation.md) for the next step (Validate and visualize atlas data).
