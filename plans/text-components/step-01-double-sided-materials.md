# Step 1 — Generic double-sided materials

Dependency: none. This must be the first implementation milestone and work on existing Composer geometry without any text infrastructure.

Follow the cross-cutting [recovery policy](recovery-policy.md); this does not request an unrelated graphics-backend exception refactor.

## Implementation

1. Inspect MaterialAttributes, MaterialSerializer/MaterialModifier, packing/revisions, Composer material commands, render queues and raster/surface/shadow consumers in the current checkout.
2. Add the `doubleSided` material value, default false. Preserve it in JSON/model round-trips, clones, snapshots, material replacement and undo/redo. Add the Composer material control now, not in the later text-editor milestone.
3. Use shared pipeline variants keyed by cull mode: Back for ordinary materials, None for double-sided materials. Existing GPU pipeline descriptions support this; prefer variants over adding dynamic-state capabilities/backend requirements. Do not create a pipeline per material or change fullscreen/overlay pass semantics.
4. Include double-sided classification in relevant material revisions/queue routing so live edits select the correct variant. Inspect every material geometry pass, including temporal surfaces, picking and shadow/visibility paths. Preserve alpha-test behavior.
5. In GLSL and Metal surface shaders, use front-facing information to orient backface shading normals coherently, including normal maps and geometric-normal consumers. Define one consistent tangent-frame policy; avoid accidentally flipping the normal twice. Handle transformed geometry consistently with existing winding conventions.
6. Inspect ray-traced visibility/shadows/reflections where enabled: correct culling and hit-normal interpretation must not contradict the material. Make only changes necessary for the requested generic material behavior; record backend limitations explicitly rather than silently omitting them.
7. Picking uses the same sidedness as visible material geometry. Do not introduce alpha-masked text picking. Existing material behavior remains unchanged with the default false.

## Likely integration points

- `lib/include/bg2e/base/MaterialAttributes.hpp` and implementation.
- Material serialization/modification, material packing and revisions in `lib/src/bg2e/{utils,draw}/`.
- `lib/src/bg2e/draw/pipeline/{GBuffer,TemporalSurfacePass}.cpp` and other applicable material passes.
- GLSL/Metal surface shaders under `shaders/src/draw/`, applicable bake/visibility shaders, and current ray-tracing paths.
- `lib/src/bg2e/manipulation/DrawPicking.cpp`, material editor and Composer material commands.

Confirm actual paths before editing. No text classes, dependencies or tutorial are implemented here.

## User handoff: Composer

Report the new control and where to find it. Ask the user to view an existing plane/thin object from both sides, toggle double sided, inspect lighting with and without a normal map, and select it from the back. Ask them to save/reopen and exercise undo/redo. Describe any applicable shadow/reflection behavior and any known limitation honestly. The user performs these observations and all builds; the agent does not execute them.

Update progress to implemented/awaiting acceptance and stop.

## Next milestone

Step 2 adds texture metadata that disables only source replacement/removal and excludes generated texture resources from serialization/export, while retaining UV and sampling controls.
