# Handoff for Generate one atlas across submeshes

Next implementation step: [step-02_atlas_modifier.md](step-02_atlas_modifier.md). Updated after completing step 01.

- Changed files and relevant API decisions: `lib/cmake/deps.cmake` now adds `${THIRD_PARTY_PATH}/xatlas/src` to the private engine dependency include list and `${THIRD_PARTY_PATH}/xatlas/src/xatlas.cpp` to the engine source list. No public header includes xatlas; no modifier/API behavior was added.
- Build command, platform and result: Not run; repository instructions prohibit compiling unless explicitly requested. Project-lead compile verification remains outstanding.
- Runtime/fixture evidence: Not applicable; no runtime or atlas behavior changed.
- Remaining limitations or regressions: Link/build integration has not yet been compiler-verified. The supplied files establish the MIT license (`Copyright (c) 2018-2020 Jonathan Young`); no source revision/provenance marker was established from the reviewed files.
- Resources, ownership and synchronization cautions for the next agent: `xatlas.h` is reachable only through the private `BG2E_THIRD_PARTY_INCLUDE_PATH` consumed by the `bg2e` target. Keep xatlas types out of public geo headers; modifier work is CPU-only and must build a transactional replacement mesh before committing.
- Exact next action: Implement the scope in step-02_atlas_modifier.md.

## Next-step instructions

Add a geo modifier for the standard Mesh. Feed all submeshes into one xatlas packing operation; preserve each submesh's triangle range and material association. Use xatlas source-vertex cross-references to duplicate vertices at seams while copying position, normal, tangent and UV1 exactly; assign only UV2. Reject multi-atlas output, invalid triangles and unrepresentable results without partial mesh mutation. Expose resolution, padding and atlas diagnostics.

## Next-step acceptance gate

CPU fixtures show exactly one nonoverlapping UV2 atlas and unchanged UV1 values after seam splits. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Add `geo::GenerateUv2AtlasModifier final : public geo::Modifier<geo::Mesh>` with `Uv2AtlasOptions { uint32_t resolution; uint32_t paddingPixels; }` and `Uv2AtlasResult { uint32_t width, height, chartCount; float utilization; }`. Expose `result() const` after `apply()`; failures throw and leave `Mesh` unchanged.
- Add each input submesh to **one** xatlas Atlas with its original vertex buffer and that submesh's index range. Generate charts and pack exactly one output atlas; set resolution and padding explicitly. Padding separates charts in UV space; this plan does not require the baker to fill those padding texels. Check `atlasCount == 1`, valid normalized coordinates and chart separation. Preserve submesh order, triangle count and material-to-submesh index.
- Build fresh arrays, mapping each xatlas output vertex through `xref` to the original source vertex. Copy position, normal, tangent and `texCoord0` exactly; set `texCoord1` from normalized xatlas UVs. Do not weld across charts. Commit both arrays and submesh ranges together only after validation succeeds.
- Test two submeshes, shared geometric vertices, existing UV2 replacement, invalid input and an atlas packing failure. UV1 must compare bit-for-bit against the corresponding original vertex. This modifier neither loads nor reloads GPU resources.
