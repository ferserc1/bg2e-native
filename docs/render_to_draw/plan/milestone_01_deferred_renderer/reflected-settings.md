# M01-12 actual settings contract

[Step](step_12_reflected-settings.md) · [Progress](../progress.md) · [Manual checks](../../../../tests/draw/settings_codec/README.md)

## Public surface and ownership

`draw::SettingsCatalog` holds targets, copied reflection metadata, availability predicates and a unique generation. `read` always invokes the active object's getter. `SettingsCodec` serializes/validates documents and creates transient `SettingsImport` commands. `RenderSettingsPreferences` adds optional borrowed `app::Preferences` persistence and opt-in delegate lifecycle/queue integration. There is no saved configuration value model or automatic loading on bind.

A `SettingsTarget` requires a stable component ID, TypeInfo, instance pointer and optional shared lifetime owner. A per-target validator sees proposed assignments merged with current getter values; a nonthrowing changed hook requests resource invalidation after the complete commit. Ordinary C++ accessors/settings remain authoritative. `TypeInfoBuilder::accessor<V>` supports typed callable getters/setters and read-only accessors; enum values retain reflection's exact int64_t representation.

Catalogs cannot be copied or moved. Reset/add assign a generation unique across catalog instances. Handles, prepared edits and imported commands are invalidated by structural changes. Retained owners keep selected layers/tools/components alive; the renderer and generator are borrowed. Clear/rebind before cleanup or any structural scene/tool replacement. Component identities use the node's persistent identifier; newly cloned nodes have different identities. Only the first enabled scene environment and enabled lights are bound, matching extraction traversal.

`connect` requires a shared facade and delegate, before the first binding. It owns the delegate's two lifecycle callbacks; applications with callbacks compose them explicitly using `bind`/`clear`. Weak callbacks clear targets before cleanup and bind actual objects after initialization/scene/resize. Queued changes capture both renderer and catalog generations, never a raw delayed object pointer. Delegate `enqueueRendererUpdate` commits at the next pre-acquire hook after draining the GPU, following pending renderer/scene changes; obsolete commands report stale status. New commands scheduled by callbacks wait for the next hook. The main-thread/stopped-background-producers contract still applies. Completion callbacks must not throw. Shutdown cancels outstanding callbacks.

Direct catalog apply/restore is suitable for CPU-only tools and stopped renderers. A running application uses queueSet/queueImport so edits request a new scene frame, including paused/noncontinuous operation. Direct callers must separately request scene redraw. SettingsResult.applied describes object mutation, not completion of GPU resource realization or durable persistence.

## Built-in instance targets

Built-in reflection is registered on first deferred binding under versioned `bg2e.draw.*.v1` type names. Registry metadata is never filtered or mutated; each target owns a copy and evaluates availability against the live graph.

| Component ID | Actual live object / properties |
|---|---|
| `renderer` | RendererDeferred queryGraphSelected and environmentError, read-only |
| `final` | Selected FinalLayer: exposure stops [-32,32], contrast [0,10], brightness [-1,1], gamma [0.1,8], debugView identities0..7 |
| `skybox` | Selected SkyboxRenderer exposure stops [-32,32] |
| `shadows` | Selected SoftShadowPass enabled/bias/directionalDistance/maxSamples1..64; editable only when query graph and actual pass are available. Available/status/eight-pair alphaTextureLimit remain discoverable read-only |
| `environment/<node ID>` | Actual EnvironmentComponent HDR source, normalized skyboxBlur, skyboxVisible; unavailable with fixed externally supplied environment resources |
| `light/<node ID>` | Actual base::Light color/intensity/type; spot controls follow current type; castShadows/sourceSize/shadowSamples0..64 require available query pass. No dormant affectsReflections control |
| `environment.generator.<role>` | Actual selected sphere/defaultSky/irradiance/specular/brdf GenerationSettings size/mipLevels/samples; actual device dimension limit, sample budget1..4096, cross-validated full mip chain; irradiance/BRDF require one mip |

Shadow distance must exceed twice bias. Final derived outputExtent/display.y are excluded. Shader paths, image format, graph selection and tool replacement remain ordinary explicit lifecycle configuration, not unchecked preferences dropdowns. Only implemented properties are exposed.

Generation edits mark `EnvironmentGenerator::requestRebuild`; RendererDeferred::prepareFrame consumes it before acquisition and publishes new resources. Source changes follow the same existing synchronize path. Failed generation retains previous resources and reports environmentError; it does not silently roll back the chosen configuration. Fix settings/source and retry. Sky exposure changes directly affect live push values. Rebind if selected tool/component objects are replaced.

Custom renderer/layer/upscaler targets use the `ExtendCatalog` callback passed to bind/connect. A custom renderer must reset the catalog with its own stable renderer ID; extensions for deferred append unique targets. They supply real getter/setter objects, owned lifetimes, dynamic options/ranges/availability and cross-validation. No closed renderer/upscaler enum is introduced; M01-13 supplies concrete upscaler registration.

## Wire format and validation

Use SettingsCodec::parse/stringify/readFile/exportFile, not the legacy JsonNode parser/writer. Schema `bg2e.draw.settings`, version string `1`, stable renderer ID, component ID/type and property names identify data. Property records contain explicit type tags and values. Numeric payloads are decimal JSON **strings**, because existing JsonNode stores ordinary JSON numbers as float. Max-digits precision preserves float/double, uint32 and sparse int64 enum identities above2^53. Booleans remain JSON booleans; strings and UTF-8 paths are escaped correctly; vectors/colors are numeric-string arrays and matrices are sixteen column-major components. Untyped number tokens, duplicate keys, bad Unicode, malformed syntax, trailing data and excessive nesting fail explicitly. Existing production JSON behavior is unchanged; only const string/bool readers were added.

Example property: `"debugView":{"type":"enum64","value":"7"}`. Display option positions never become configuration values.

Validation checks schema/version/renderer/component type, shapes, exact storage tags/any types, finite values, ranges, enum identities, writable/available properties, duplicates and complete cross-property constraints before any setter runs. Default policy rejects unknown or unavailable properties. Explicit SkipUnknownOrUnavailable returns warnings and skips them; malformed typed values still fail even in skipped fields. Missing properties preserve current live values. Availability is evaluated against the current active object: switch a light type first, refresh the catalog view, then edit newly available controls. Immutable derived/read-only fields always reject writes. Unknown extra object fields are ignored for forward-compatible annotations; they do not bypass validation of known fields.

Apply rechecks generation and validation at commit. It reads original values before writing and attempts reverse rollback, including the failing setter, if a setter throws. Custom setters must be reversible and throw standard exceptions; arbitrary external effects cannot be made atomic by reflection. Rollback failures are explicit issues. Post-commit changed-hook failures report an error with applied=true, rather than claiming values were unchanged. Built-in changed hooks only mark rebuild intent.

## Persistence and precedence

Preferences is optional and borrowed; it must outlive the facade. persist reads actual getters and writes the codec document as ASCII hex under `draw.settings.v1.hex` (custom key supported). Hex avoids the legacy Preferences writer's unescaped-string behavior without changing that production service. save retains the service's existing log-only I/O error reporting; successful return does not independently confirm durability.

restore(explicitDocument) applies only that document and bypasses persisted defaults. Otherwise restore decodes the persisted document; no saved entry is a valid no-op. Missing properties retain live values. Invalid stored data is diagnosed without fallback/coercion or partial writes. Headless callers instantiate only the catalog/codec, or optionally inject Preferences; no Engine, window or UI is required.

## Verification and successor work

Agent verification was nonexecuting: local includes, delimiters, exact codec type tags, validation-before-write ordering, generation/weak ownership paths, actual ordinary accessor declarations, dependency boundaries, local documentation links and git diff --check. No configuration, compilation, shader compilation or runtime checks were performed. The CPU fixture and persistence/deferred lifecycle scenarios are for the project lead.

M01-13 appends actual upscaler/scale targets and synchronizes any resource changes with lifecycle rebinding. M01-14 renders descriptors and uses live reads plus queued commands; widgets retain handles/generations, never descriptor-derived target pointers or mirrored configuration. M01-15 demonstrates persistence/import/export and replacement using the same facade. M03/M04 register implemented properties through extensions. M05 consumes the same codec directly without UI initialization.
