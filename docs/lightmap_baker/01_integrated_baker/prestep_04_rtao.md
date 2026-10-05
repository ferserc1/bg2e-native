# Handoff for Implement RTAO baking

Next implementation step: [step-04_rtao.md](step-04_rtao.md). Complete this file **after** finishing step 03; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in step-04_rtao.md.

## Next-step instructions

Adapt the AO pass to consume UV-space surface data at native map resolution; preserve the existing screen-space entry points. Produce an AO result through the common baker interface. Keep projected shadows independent and disabled for the model_edit preset. Ensure valid-texel masking and stable output on an AO fixture.

## Next-step acceptance gate

RTAO bake produces a nonempty full-resolution target on a fixture with valid UV2. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Extract or overload the existing RTAO traversal so the new entry point reads `UvSurfacePass` world position/normal/mask instead of reconstructing a hit point from screen depth and inverse camera matrices. Preserve `RTAmbientOcclusion::render(..., GBufferManager*, inverseViewProjection)`.
- At UV resolution, shoot the configured number of rays per valid texel against the prepared TLAS. Respect ray bias and `maxRayDistance`; use existing alpha-tested hit behavior. Write a linear single-channel visibility value, neutral 1 on uncovered texels.
- The mode-specific baker selects RTAO only; it does not run RTGI. Shadow toggling is handled by the later composition step, not by changing AO radius or indirect-shadow samples.
- Verify a plane with an overhanging occluder darkens only expected mapped texels; an isolated plane remains close to neutral. All submeshes share the atlas.
