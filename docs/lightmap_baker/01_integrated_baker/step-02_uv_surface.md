# Step 02: Build the UV-space surface pass

## Scope

Rasterize the complete target Drawable, including every submesh and its transform, into full-resolution UV2-space surface data (world position, normal, material/submesh identity and valid-texel mask). Keep UV1 material sampling intact. Define atlas overlap and missing-UV2 rejection. Ensure texture padding and empty texels are distinguishable. Add shaders and Vulkan resources needed for this pass.

## Acceptance and compile gate

A diagnostic result can identify covered texels and submeshes on a known UV2 fixture; code compiles. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Handoff

After this step, complete [prestep_03_scene_bindings.md](prestep_03_scene_bindings.md) for the next step (Reuse frame RT data safely).
