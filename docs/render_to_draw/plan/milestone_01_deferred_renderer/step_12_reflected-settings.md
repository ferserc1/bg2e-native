# M01-12 — Stateless preferences facade, dynamic catalog and settings codec

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M01-05/M01-10/M01-11.

## Scope and deliverable

- `lib/include/bg2e/reflection/Property.hpp`
- `lib/include/bg2e/reflection/Builder.hpp`
- `lib/include/bg2e/reflection/TypeInfo.hpp`
- `lib/include/bg2e/draw/RenderSettingsPreferences.hpp`
- `lib/src/bg2e/draw/RenderSettingsPreferences.cpp`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [lib/include/bg2e/reflection/Property.hpp](../../../../lib/include/bg2e/reflection/Property.hpp), [lib/include/bg2e/reflection/Builder.hpp](../../../../lib/include/bg2e/reflection/Builder.hpp), [lib/include/bg2e/reflection/TypeInfo.hpp](../../../../lib/include/bg2e/reflection/TypeInfo.hpp).

## Implementation work

1. Bind reflection accessors to active renderer/layer/environment/upscaler objects for UI-facing properties. Keep ordinary object accessors and configuration as the sole values; the facade stores only targets/metadata/persistence plumbing.
2. Expose instance-specific enumeration with type/category/range/enum options and capability visibility. Use a catalog generation to invalidate UI handles after renderer/component changes; leave shared TypeRegistry metadata intact.
3. Implement persistence plus versioned JSON serialize/deserialize/export/import using reflected getters/setters, exact any types and stable property/component identifiers. Enumerated options are identities, not list indices.
4. Validate complete import before mutation and apply through object setters at a safe boundary. Specify unknown/unavailable property diagnostics and persisted-default versus explicit-file precedence without retaining a shadow value model.

## Verification and exit conditions

- Programmatic setter changes are immediately visible to UI/export; imported values and persistence change the same object and survive a restore round-trip.
- CPU-only codec checks cover malformed types/enums/ranges and unsupported properties; renderer replacement clears stale property references.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M01-13/M01-14 complete upscaler/UI registration. M03/M04 add properties; M05 uses the same settings codec with no UI.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-05 lifecycle handoff

Use `DefaultRenderLoopDelegate::setRendererChangeCallbacks` to detach before cleanup and bind after initialization/scene attachment/resize. Install these callbacks before initial binding. Queued UI/settings actions capture weak delegate ownership plus `rendererGeneration` and use `withRenderer`; a SafeUpdateToken or GPU wait alone does not validate an obsolete raw target. See [runtime-renderer.md](runtime-renderer.md).

## M01-08 live environment settings

See [environment-tools.md](environment-tools.md). GenerationSettings live on public selected tool objects and apply on explicit safe rebuild; SkyboxSettings exposure is a live push value. EnvironmentComponent source/normalized blur/visibility are declarative scene state. Settings bindings must not invoke synchronous generation inside recording; queue source/config rebuilds before acquisition and rebind published snapshots. No parallel preferences copy of these settings is needed.

## M01-10 live setting targets

Use [raster-layers.md](raster-layers.md): RendererDeferred owns live RendererConfiguration.layers, explicit/scene camera selection and environmentError(); FinalLayer owns FinalSettings (exposure stops/contrast/brightness/positive gamma and implemented debug selection0..7). display.y and outputExtent are derived ABI fields, not user preferences. finalSettings() only works with a FinalLayer-derived selection; injected custom final passes expose their own reflected catalog. Ordinary environment replacement uses Renderer::prepareFrame via DefaultRenderLoopDelegate before acquisition; record/update must never generate HDR resources. Bind actual pass instances and requestSceneFrame after noncontinuous edits; do not invent dormant effect settings.

## M01-11 query settings and availability

Use the [actual query pass](query-shadows.md): RendererConfiguration.softShadows retains an injectable SoftShadowPass; RendererDeferred exposes softShadows() and usesQueryShadows(). Reflect live SoftShadowSettings enabled/bias/directionalDistance/maxSamples and existing per-light castShadows/sourceSize/shadowSamples. Distinguish graph selection, pipeline available/status, enabled setting, effective device-clamped alphaTextureLimit and scenes with no occluders/lights; do not infer support from fullSupported or render-stage capability. maxSamples1..64 caps per-light samples (zero light samples uses a hard ray). The original eight-pair limit is superseded by the shared 16384/device budget. Change resource/graph choices only at a stopped lifecycle boundary; normal disabled frames need no Drawable BLAS or TLAS.

## M01-12 delivered contract

See [reflected-settings.md](reflected-settings.md) for SettingsCatalog/SettingsCodec/SettingsImport, live built-in targets and extension callbacks, unique generations, weak delegate lifecycle integration, pre-acquire queued commits, precision-preserving typed JSON and hex Preferences persistence. CPU-only manual fixtures are in tests/draw/settings_codec; no build or runtime checks were run by the agent. M01-13 supplies actual upscaler targets through the extension surface.
