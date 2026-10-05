# Step 01: Define the public contract

## Scope

Add render API types for bake settings, RGB8/RGB32F result selection, an IntegratedBakerContext, and a per-target LightmapBaker. The context refers to one scene root; each baker owns its target image, intermediate images, accumulation history, and CPU result. Define lifetime, state transitions, RT-required errors, node-root and Drawable validation, full-resolution extent, and invalidation rules. The integrated update takes current-frame resources explicitly and never retains a borrowed TLAS. No rendering implementation is required yet.

## Acceptance and compile gate

Public headers and inert implementations compile; existing render API signatures remain intact. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Handoff

After this step, complete [prestep_02_uv_surface.md](prestep_02_uv_surface.md) for the next step (Build the UV-space surface pass).
