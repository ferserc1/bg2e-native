# Step 05: Implement RTGI and optional RT shadows

## Scope

Adapt GI and direct shadow evaluation to UV-space texels and the context-owned scene TLAS prepared from the root. Select exactly one of RTAO or RTGI; compose optional RTShadows into the single lightmap result for either choice. Define one RGB meaning for the composed AO texture and document the formula in code-level API comments. Keep all passes at native map resolution.

## Acceptance and compile gate

All four mode/shadow combinations execute and compile without changing the current renderer API. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- Add a UV-space RTGI path taking the selected `UvSurfacePass` manager's position/normal/mask images, the context-owned TLAS, material/instance bindings, environment and light list. Do not pass this depthless manager to the existing screen-space path, which expects camera depth and inverse view-projection; preserve the existing `RTGlobalIllumination::render` signature. GI bounce count, samples and max distance come from `LightmapSettings`; dispatch at atlas resolution.
- Add a distinct RT shadow visibility pass that can be enabled for either mode. The four combinations are RTAO/no shadows, RTAO/shadows, RTGI/no shadows and RTGI/shadows. Disabled shadows contribute a neutral factor of 1 and do not launch shadow rays.
- Add one composition pass that converts RTAO or RTGI into the RGB light multiplier defined in [API_CONTRACT.md](../API_CONTRACT.md), applies shadow visibility once, and writes the same target image in every combination. Neutral white, fully occluded black and colored indirect lighting are required fixture cases.
- Update the material/render shader path so AO textures are sampled as RGB, not only `aoChannel`: derive an internal baked flag from explicit AO texture presence, add a sixth attachment to `GBufferManager`'s default camera profile without shifting existing indices or changing the UV profile's four attachments, and apply the RGB multiplier once to the non-emissive lit result. For baked objects, bypass live RTGI/RTAO and live RT shadows in composition; emission remains separate. Apply the same rule to the non-RT composite. Do not add a serialized material property or touch emission settings.
- Validate existing grayscale AO textures still load; RGB channels must remain distinguishable in a rendered colored test map. Keep current `RendererDeferred` public API intact.

## Review matrix

| Mode | RT shadows | Expected output |
|---|---:|---|
| RTAO | off | gray occlusion |
| RTAO | on | gray occlusion times shadow visibility |
| RTGI | off | colored indirect factor |
| RTGI | on | colored indirect factor times shadow visibility |

## Handoff

After this step, complete [prestep_06_accumulation_output.md](prestep_06_accumulation_output.md) for the next step (Add accumulation and CPU/GPU results).
