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

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Add a UV-space RTGI path taking the same surface buffer, TLAS, material/instance bindings, environment and light list; preserve the existing screen-space `RTGlobalIllumination::render` signature. GI bounce count, samples and max distance come from `LightmapSettings`; dispatch at atlas resolution.
- Add a distinct RT shadow visibility pass that can be enabled for either mode. The four combinations are RTAO/no shadows, RTAO/shadows, RTGI/no shadows and RTGI/shadows. Disabled shadows contribute a neutral factor of 1 and do not launch shadow rays.
- Add one composition pass that converts RTAO or RTGI into the RGB light multiplier defined in [API_CONTRACT.md](../API_CONTRACT.md), applies shadow visibility once, and writes the same target image in every combination. Neutral white, fully occluded black and colored indirect lighting are required fixture cases.
- Update the material/render shader path so AO textures are sampled as RGB, not only `aoChannel`: derive an internal baked flag from explicit AO texture presence, add a sixth G-buffer color attachment without shifting existing indices, and apply the RGB multiplier once to the non-emissive lit result. For baked objects, bypass live RTGI/RTAO and live RT shadows in composition; emission remains separate. Apply the same rule to the non-RT composite. Do not add a serialized material property or touch emission settings.
- Validate existing grayscale AO textures still load; RGB channels must remain distinguishable in a rendered colored test map. Keep current `RendererDeferred` public API intact.
