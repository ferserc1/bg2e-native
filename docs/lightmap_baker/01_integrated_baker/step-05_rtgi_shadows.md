# Step 05: Implement RTGI and optional RT shadows

## Scope

Adapt GI and direct shadow evaluation to UV-space texels and the scene TLAS. Select exactly one of RTAO or RTGI; compose optional RTShadows into the single lightmap result for either choice. Define one RGB meaning for the composed AO texture and document the formula in code-level API comments. Keep all passes at native map resolution.

## Acceptance and compile gate

All four mode/shadow combinations execute and compile without changing the current renderer API. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Handoff

After this step, complete [prestep_06_accumulation_output.md](prestep_06_accumulation_output.md) for the next step (Add accumulation and CPU/GPU results).
