# Handoff for Build the UV-space surface pass

Next implementation step: [step-02_uv_surface.md](step-02_uv_surface.md). Complete this file **after** finishing step 01; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in step-02_uv_surface.md.

## Next-step instructions

Rasterize the complete target Drawable, including every submesh and its transform, into full-resolution UV2-space surface data (world position, normal, material/submesh identity and valid-texel mask). Keep UV1 material sampling intact. Define atlas overlap and missing-UV2 rejection. Ensure texture padding and empty texels are distinguishable. Add shaders and Vulkan resources needed for this pass.

## Next-step acceptance gate

A diagnostic result can identify covered texels and submeshes on a known UV2 fixture; code compiles. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.
