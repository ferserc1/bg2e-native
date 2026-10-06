# Step 04: Implement RTAO baking

## Scope

Adapt the AO pass to consume UV-space surface data at native map resolution; preserve the existing screen-space entry points. Produce an AO result through the common baker interface. Keep projected shadows independent and disabled for the model_edit preset. Ensure valid-texel masking and stable output on an AO fixture.

## Acceptance and compile gate

RTAO bake produces a nonempty full-resolution target on a fixture with valid UV2. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- Extract or overload the existing RTAO traversal so the new UV entry point reads the selected `UvSurfacePass` manager's position, normal and mask images directly, together with the context-owned TLAS. Do not pass the UV manager to the existing camera-space entry point: that method dereferences `depthImage()` and reconstructs position from inverse view-projection. Preserve `RTAmbientOcclusion::render(..., GBufferManager*, inverseViewProjection)` unchanged.
- At UV resolution, shoot the configured number of rays per valid texel against the context-owned TLAS selected by `prepareFrame`, never `frameResources.rayTracingScene`. Respect ray bias and `maxRayDistance`; use existing alpha-tested hit behavior. Write a linear single-channel visibility value, neutral 1 on uncovered texels.
- The mode-specific baker selects RTAO only; it does not run RTGI. Shadow toggling is handled by the later composition step, not by changing AO radius or indirect-shadow samples.
- Verify a plane with an overhanging occluder darkens only expected mapped texels; an isolated plane remains close to neutral. All submeshes share the atlas.

## Compatibility boundary

Do not change the screen-space RTAO settings or existing shader entry point behavior.

## Handoff

After this step, complete [prestep_05_rtgi_shadows.md](prestep_05_rtgi_shadows.md) for the next step (Implement RTGI and optional RT shadows).
