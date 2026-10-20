# Environment generation and HDR precision

Public headers live under `<bg2e/draw/environment/>`; `all.hpp` includes the
family. `EnvironmentResources` retains sky, irradiance, prefiltered specular,
BRDF LUT and samplers by shared ownership. `validate(device)` checks compatibility.
Engine/device outlive every snapshot user. Linear HDR sky radiance, irradiance
including PI, GGX-prefiltered roughness mips and the split-sum RG LUT have different
meanings; do not gamma-correct them as ordinary color textures.

`EnvironmentGenerator(Engine&, shaderRoot)` synchronously builds resources from
an HDR path or a linear equirectangular GPU image. Empty path builds the default
sky. Its public configuration accepts replacement sphere/default-sky/irradiance/
specular/BRDF `CubemapRenderer` objects. `requestRebuild()` marks intent;
`synchronize(EnvironmentComponent*)` consumes changes at a safe boundary.
A failed build preserves the last published snapshot. Stop producers during
build, synchronization and cleanup. RendererDeferred exposes generator/status
access for live environment settings.

The concrete tools are `SphereToCubemapRenderer`, `IrradianceCubemapRenderer`,
`SpecularReflectionCubemapRenderer`, `BRDFIntegrationMapTextureGenerator` and
`SkyboxRenderer`. They use explicit face/mip targets and paired shaders. Specular
roughness mips are authored separately: generic mip generation would overwrite
their meaning. Skybox blur selects normalized prefiltered roughness, not a second
color transfer operation.

HDR file loading rejects nonfinite source pixels and promotes to float32 when
maximum RGB magnitude times PI exceeds 65000. Float32 propagates through realized
sky/irradiance/specular generation; requested settings are restored afterwards.
This avoids float16 overflow before convolution without clamping radiance.
`autumn_field_4k.hdr` exercises this case. The project lead confirmed the precision
correction; this does not establish every backend/environment combination.

See [GPU images](../gpu/Subresources_and_rendering.md) and
[deferred renderer](RendererDeferred.md).

## Procedural sky colors

`GenerationSettings` exposes three linear RGB colors for `ProceduralSkyRenderer`:

| Setting | Default RGB |
| --- | --- |
| `skyHorizonColor` | `(0.55, 0.65, 0.8)` |
| `skyZenithColor` | `(0.06, 0.22, 0.6)` |
| `groundColor` | `(0.04, 0.035, 0.025)` |

RGB components must be finite and within `[0, 1]`; alpha is ignored. Both GLSL and Metal use these values. The existing horizon transition and sun remain unchanged.

`scene::EnvironmentComponent` exposes matching getters and setters, reflection color editors, and JSON fields. Missing JSON fields restore the original defaults. Colors are stored with scenes and environment presets, including when an HDR image is selected, and take effect when the image path is empty. `EnvironmentGenerator::synchronize()` detects color changes and rebuilds procedural sky and IBL resources at the renderer update boundary. Direct generation users can change the colors through `configuration().defaultSky->settings()` and call `build()` again.

Draw Model Edit's Environment Options uses `ui::ReflectionWidget::drawProperties()` on a cloned `EnvironmentComponent`, sharing the component editor's metadata and controls. Changes are queued through the editor's existing environment update operation before the edited component is adopted.

## Composer active environment

Composer opts into `scene::Scene::setMainEnvironment(component)`; explicit nullptr
means renderer defaults, not another Environment found during traversal. Selection
uses weak component/node owners, validates enabled scene membership, and clears on
root release/replacement. Existing Scene clients that do not opt in retain their
automatic first-environment behavior. Draw renderer generation, queue snapshots and
live settings honor the explicit selection, including the intentional null case.

Composer has no hidden Environment on its auxiliary stage root. Opening a scene
selects its first enabled Environment (if several exist). Subsequent additions are
ignored while a valid component is active. Deleting the active component selects
default sky; previously ignored components are not promoted. A newly added enabled
component can become active once there is no selection. These runtime choices are
not an extra serialized active-ID format: opening a document starts a fresh choice.
Fresh Open resets the generator's source cache and requests regeneration even when
the HDR path matches the previous document, so cached resources cannot conceal
updated file contents.

Built-in Environment inspector edits apply to the same authored instance at the
stopped boundary. HDR selection and clearing preserve the stored procedural RGB;
colors affect sky/IBL only when the HDR reference is empty. Inactive components are
labeled and do not regenerate or publish GPU resources. Only renderer synchronization
realizes the selected component. Both legacy `equirectangularTexture` and the
`environmentImage` property alias load; a missing/empty reference clears prior HDR.
The reflection picker has distinct Browse and Clear buttons. Clear or explicit
empty selection removes the HDR source; cancelling Browse preserves it.

Images must contain floating-point HDR data; an SDR JPEG is not an HDR merely
because it appears in an Environment component. Failure retains the last good GPU
snapshot and reports `environmentError` in Composer status/inspector. It does not
silently replace the requested component or discard the user's edited colors.
