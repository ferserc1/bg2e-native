# M01-15 — Build and adapt examples/draw/10_deferred_renderer

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M01-01–M01-14.

## Scope and deliverable

- `examples/17_deferred_renderer/src/main.cpp`
- `examples/draw/README.md`
- `examples/CMakeLists.txt`
- `examples/draw/10_deferred_renderer/src/main.cpp`
- `examples/draw/10_deferred_renderer/CMakeLists.txt`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [examples/17_deferred_renderer/src/main.cpp](../../../../examples/17_deferred_renderer/src/main.cpp), [examples/draw/README.md](../../../../examples/draw/README.md), [examples/CMakeLists.txt](../../../../examples/CMakeLists.txt).

## Implementation work

1. Adapt the reference scene: OBJ two-submesh models with textures, opaque/transparent variants, floor, nested transforms, rotating camera/lights and HDR environment through draw Drawable and the new non-template delegate.
2. Retain supported material/sky/debug controls and add reflected settings, default scale/optional FSR selection and query soft-shadow controls. Remove only out-of-milestone AO/GI/reflection/denoise/manipulation/bake/offscreen controls.
3. Use MainLoop EngineConfig selection and paired pipeline shader libraries. Register a unique executable target/bundle and assets; do not implement another application loop.
4. Include controllable cutout/no-RT fixtures and a safe same-type renderer replacement action. Keep an explicit reference checklist of removed/deferred capabilities so M03 can restore effect controls.

## Verification and exit conditions

- The requested example runs on both available backends, resizes/pauses/resumes correctly and renders without RT/FSR.
- Clone/edit independence, material channels, query cutouts, settings-file round-trip and external upscaler injection have concrete evidence.
- No manipulation/app/bake/headless feature is required to run the example.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M02/M04/M05 derive their products from this scene. M03 updates this existing target rather than create a new example.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-05 lifecycle handoff

Use the implemented non-template `draw::DefaultRenderLoopDelegate` and its queued `setRenderer` action for the real RendererDeferred-to-RendererDeferred replacement. M01-05's shader-free lifecycle probe is only the intermediate check; this product must demonstrate M01-12 property detachment/rebinding, retained scene identity and stale UI-action rejection. Use `setContinuousRendering(true)` for the reference animation and sceneFrameNumber/sceneDeltaSeconds for scene timing. See [runtime-renderer.md](runtime-renderer.md).

## M01-07 loader handoff

Use db::loadScene(path, draw::Engine&, callback), loadDrawableBg2(path, draw::Engine&, callback) or loadDrawableObj(path, draw::Engine&). BG2 carries material/submesh metadata; OBJ currently supplies geometry only. Scene references accept explicit path and legacy name-to-BG2 resolution. EnvironmentComponent retains the HDR source for M01-08. Schema-only loadScene(path) does not open any referenced asset; Node::loadResources(engine) realizes it later. saveSceneReferences writes references only, not edited/generated resource contents. See [db-levels.md](db-levels.md).

## M01-08 environment baseline

Example 06 now consumes public EnvironmentGenerator/SkyboxRenderer. Reuse [environment-tools.md](environment-tools.md) for the deferred example: HDR/default sky, scene source changes and blur/visibility, shared validated IBL, linear/PIs/BRDF conventions and safe replacement. Validate asymmetric axes/seams and no-RT behavior; do not copy the historical example's old generation shaders into a second private algorithm family.

## M01-10 runnable integration basis

RendererDeferred is now concrete; use its [actual graph/lifecycle](raster-layers.md) and [manual fixture](../../../../tests/draw/deferred/README.md). DefaultRenderLoopDelegate invokes the pre-acquire prepareFrame hook for environment changes. A supplied fixed environment bypasses generation; environmentError reports failed source replacement. Demonstrate application-owned compatible pass injection, renderer replacement and the actual live final settings/upscaler integration from later steps. Preserve six texture slots, both transparent classes, copied opaque depth, color correction once and separate internal/output extents. Do not expose unimplemented effects.

## M01-11 query product basis

Demonstrate [SoftShadowPass](query-shadows.md) with actual cutout UV/cutoff/sampler and direct-light softness controls. Default rendering uses compute queries plus the query-aware deferred variant when successfully available, including Metal without render-stage queries; no-RT/kernel-unavailable keeps unshadowed PBR. Explicit unshadowed lighting injection opts out. settings.enabled=false works with Drawable tracing disabled and no TLAS. Show pipeline availability/status/selected graph separately, retain per-slot owners and do not expose unimplemented advanced effects. Preserve the lead's corrected irradianceImage shader name and public CommandBuffer include.

## M01-12 actual settings integration

Use a shared RenderSettingsPreferences connected before the delegate's first binding; see [reflected-settings.md](reflected-settings.md). Demonstrate live ordinary setters, queued UI/import edits, explicit persistence/restore precedence, and stale queued-action rejection across renderer replacement, scene attachment and resize. Custom targets append through ExtendCatalog. Generation edits request rebuild only at prepareFrame; successful settings mutation does not imply successful environment realization. The CPU fixture and manual persistence/lifecycle checks are documented in tests/draw/settings_codec/README.md.

## M01-13 actual reconstruction integration

Read [upscaling.md](upscaling.md) and tests/draw/upscaling/README.md. Demonstrate live ResolutionSettings enabled/scale, FsrUpscaler injection and enabled/sharpness/status with bilinear fallback on Metal/no-SDK. Include the application-defined Upscaler fixture without adding engine enum identities. RendererDeferred final replacements use finalContract(output, depth, true), with output-sized linear HDR and internal debug inputs. Validate native/reduced/odd sizes, debug/sRGB color parity, opaque-depth camera motion, scene/extent/cut resets, UI-only presentations and fresh catalog generations. FSR moving-object/glass limits remain explicit until M03.

## M01-14 actual settings window

Use [GraphicsSettingsWindow](settings-ui.md): bind a shared RenderSettingsPreferences connected before first delegate binding, then draw from the normal UserInterfaceDelegate. Demonstrate safe queue edits/import, optional save/restore, file export and pending/stale/availability diagnostics through the actual window. No copied UI configuration, legacy RenderSettingsWindow or Vulkan TextureWidgets is needed. File intent uses ResourcePicker; full material/preview widgets remain later steps. See tests/draw/settings_ui/README.md.

## M01-15 implementation handoff

Actual target, CLI, scene/material fixtures, extension/replacement/lifecycle contracts and lead checks are recorded in [working-example.md](working-example.md). The new target uses only existing paired engine shaders/assets. FinalLayer now declares reconstructedColor through its constructor before injection validation. M01-16 is the next separate review step after lead acceptance.
