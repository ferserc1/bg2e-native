# M03-11 — Update cumulative implementation documentation

[Milestone summary](summary.md) · [Plan index](../index.md) · [Documentation policy](../documentation-policy.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see progress. **Prerequisites:** Accepted M03-10 technical review and the implemented product/corrections it records. This is a separate required final step, not implemented by adding this plan file.

## Scope and deliverable

Update maintained `doc/` API/reference, conceptual, tutorial, application and build/distribution pages affected by M01–M03, including indexes and retained pages with affected links/examples. Produce `documentation.md` in this milestone directory only during execution, mapping changed/retained/retired pages to actual source, examples and validation. Historical handoff/review files do not replace user documentation.

## Milestone coverage: Delivered ray-tracing effects, histories and denoising

Actual accepted backend RT data/binding design; capability versus SDK/runtime support; BlueNoise, RTAO/RTGI/reflections, temporal histories/reset and denoise; public effect injection and reflected settings; update the existing deferred/manipulation tutorials. Describe only delivered effects/backend coverage; do not present future baking or offscreen APIs as available.

## Implementation work

1. Follow the [cumulative documentation policy](../documentation-policy.md). Read actual source, accepted corrections and this milestone review; update the previous documentation baseline instead of writing a parallel future API reference.
2. Inventory affected maintained pages and reconcile actual signatures, namespaces, lifecycle/ownership, reflected settings/schema, capabilities, extension hooks, examples/assets/shaders and deployment. Update or retire contradictory guidance and navigation.
3. Add usable reference and focused tutorials/application instructions for the functionality delivered so far. Reuse actual source/example call sites; name backend/SDK limits and evidence honestly. No snippets or usage guides for unimplemented later milestones.
4. Check links/anchors, symbol/header/target/resource paths, snippets and diagrams statically. Preserve accurate foundation pages and the current coexistence/production status. Compilation/runtime validation stays with the project lead unless explicitly requested.
5. Record coverage/dispositions, source mappings, checks, limitations and remaining documentation obligations in `documentation.md`; append the documentation handoff to `review.md` without rewriting its earlier history. Reconcile affected pending steps and progress.

## Exit and transition

- Maintained documentation describes the actual cumulative implementation through M03, with no future feature presented as available.
- Relevant API reference, tutorials, app/CLI/build guidance and navigation are updated; all applicable former M10 authoring requirements are covered or explicitly assigned to an unimplemented milestone owner.
- Source/links/snippet checks and actual validation ownership are recorded; no invented compilation/backend coverage.
- Deliver this documentation step for lead acceptance before entering M04. Follow the normal one-step execution protocol. M10 audits the accumulated result and does not defer missing authoring work to itself.

## M03-07 actual temporal handoff

Use [temporal.md](temporal.md): public TemporalAccumulator returns owned sampled scalar/HDR output or borrowed raw pass-through; SurfaceInputs/TemporalView expose inputs without private renderer access. Keys separate signal/view/layer, sceneFrameNumber advances actual work, maxFrames bounds per-pixel arithmetic/rolling/frozen accumulation. GI binary validity and reflection fractional confidence remain distinct. Renderer::invalidateHistories coordinates SDK and engine histories; resetUpscaler forwards for existing adapters. Exact extraction/settings signatures detect identity/value changes; in-place uploads/custom shaders and same-camera teleports require app notification. Object motion conservatively resets until previous-object vectors are added. Fresh history images and per-slot retained versions avoid overwrite; same ordered queue/submission discipline is required.

## M03-08 actual filter handoff

Use [denoise.md](denoise.md): RendererDeferred injects/exposes DenoiseFilter and resolves tracing -> temporal -> spatial -> lighting. Inputs reuse SurfaceInputs; output keeps scalar R16/HDR RGBA16 and independent source extent. GI validity and reflection confidence are explicit; filtered RGB stays unpremultiplied and never replaces temporal history. Camera thresholds use linear view Z, explicit profiles use world distance. Virtual settingsType/settingsInstance accessors support live catalog binding; M03-09 must register real controls/preferences and avoid copied settings values. Unsupported profiles return borrowed source; fresh output owners and per-slot descriptors remain alive through completion.

## M03-09 concrete integration handoff

Use [integration contracts](../milestone_03_ray_tracing/integration.md): live
requested effect settings persist independently of capability status; renderer
mode selects IBL/AO/GI separately from reflections. Existing examples inject
optional disabled FSR and SMAA. FinalProcessor/SMAAProcessor apply final scene AA
after color/reconstruction and before overlays/UI, with standalone borrowed/owned
output and retired-slot contracts. Reuse shared noise and explicit invalidation.
The later M07 correction expands direct query shadows to the device-clamped 16384
policy; overflow degrades only alpha texture slots, never the entire scene's
shadows. Review no-query/no-array/partial support and the manual matrix.
Keep official AreaTex/SearchTex bytes and original licenses in iryku_smaa during
production retirement: draw SMAA now consumes those resources. Do not use spatial
AA or denoised histories as bake averages. Runtime checks remain lead-owned.

## M03-10 reviewed implementation requirement

Reconcile the real startup/reopen graphics windows and explicit facade ownership in both examples, example-only active GI/reflection defaults and local lightPose targets. Cover SurfaceInputs formats, normalization/confidence, bounded histories versus exact averaging and optional post-color FinalProcessor/SMAA. Preserve lead evidence limits; do not present baking/headless APIs as implemented.

Source and validation boundaries: [M03 technical review](../milestone_03_ray_tracing/review.md). Include these conditions in this step's implementation and exit checks.
