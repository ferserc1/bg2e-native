# Handoff for Implement RTGI and optional RT shadows

Next implementation step: [step-05_rtgi_shadows.md](step-05_rtgi_shadows.md). Complete this file **after** finishing step 04; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in step-05_rtgi_shadows.md.

## Next-step instructions

Adapt GI and direct shadow evaluation to UV-space texels and the scene TLAS. Select exactly one of RTAO or RTGI; compose optional RTShadows into the single lightmap result for either choice. Define one RGB meaning for the composed AO texture and document the formula in code-level API comments. Keep all passes at native map resolution.

## Next-step acceptance gate

All four mode/shadow combinations execute and compile without changing the current renderer API. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.
