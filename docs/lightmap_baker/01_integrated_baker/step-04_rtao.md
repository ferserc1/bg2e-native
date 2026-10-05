# Step 04: Implement RTAO baking

## Scope

Adapt the AO pass to consume UV-space surface data at native map resolution; preserve the existing screen-space entry points. Produce an AO result through the common baker interface. Keep projected shadows independent and disabled for the model_edit preset. Ensure valid-texel masking and stable output on an AO fixture.

## Acceptance and compile gate

RTAO bake produces a nonempty full-resolution target on a fixture with valid UV2. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Handoff

After this step, complete [prestep_05_rtgi_shadows.md](prestep_05_rtgi_shadows.md) for the next step (Implement RTGI and optional RT shadows).
