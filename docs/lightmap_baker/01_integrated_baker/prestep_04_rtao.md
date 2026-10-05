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
