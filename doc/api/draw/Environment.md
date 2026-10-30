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

RGB components must be finite and within `[0, 1]`; alpha is ignored. Both GLSL and Metal use these values. Without an associated solar light, the original horizon transition and fixed sun are preserved.

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


## Solar association

Place an `EnvironmentComponent` and a `LightComponent` with
`light().setType(base::Light::TypeSun)` on the **same node**. The active procedural
Environment resolves that node's light; no node ID, child search or serialized
cross-reference is introduced. Rotating the node moves the sun. The horizon uses
world `+Y` and does not rotate with the node. Translation affects gizmo placement
only. Camera gizmos retain their existing priority; a Sun gizmo takes priority
over the Environment gizmo.

Composer's explicit active Environment selection still applies. Inactive
Environments do not change the active sky or atmospherically filter their lights.
A disabled node or ancestor excludes the association. A missing solar light keeps
the original fixed procedural sun. Selecting an HDR image disables the procedural
solar association without deleting the solar properties; clearing HDR restores it.
The Sun then remains a directional emitter with physical size and distance scaling
but receives no atmospheric modifier while HDR is selected.

Configure dimensions on `base::Light`, not the Environment:

| Property | UI label | Default |
| --- | --- | --- |
| `solarSize` | Solar Size (1 = one solar radius) | 1.0 |
| `solarDistanceMillionKm` | Solar Distance (million km) | 149.6 |
| `intensity` | Intensity | 1, existing engine units |
| `color` | Color | White, linear RGB |

The default dimensions produce an angular radius of approximately 0.266 degrees.
Distance is measured in millions of kilometers independently of scene units, and
radius is measured in kilometers. Distance also scales intensity by inverse square
relative to 149.6 million km. See [Light](../base/Light.md) for validation,
serialization and effective light values.

## Terrestrial solar approximation

`base::evaluateSolarLight()` integrates 64 midpoint samples through a spherical
atmospheric shell. The internal profile uses a 6360 km planet, a 60 km integration
shell, an observer 1 m above its surface, an 8 km Rayleigh density scale height and
a 1.2 km aerosol scale height. RGB extinction uses approximate representative
wavelengths and Beer–Lambert transmission. These are fixed model assumptions;
the shell is a numerical cutoff, not a sharp physical atmosphere boundary.
The starting profile follows the [Bruneton terrestrial model example](https://ebruneton.github.io/precomputed_atmospheric_scattering/atmosphere/demo/demo.cc.html).
Solar dimensions follow [NASA](https://nssdc.gsfc.nasa.gov/planetary/factsheet/sunfact.html?level=1)
and [ESA](https://www.esa.int/Science_Exploration/Space_Science/The_Sun) reference values.

The longer near-horizon optical path attenuates blue more strongly than red.
Effective direct illumination fades using the fraction of the solar disk above
the horizon, reaching zero when it is fully below it. No additional elevation
cosine is applied to light intensity; surface orientation is handled by lighting.
Authored light color/intensity and environment colors remain unchanged.

The sky uses the existing horizon, zenith and ground colors as its daytime basis.
Elevation controls daylight, a warm horizon transition and a sunward twilight
accent. A dim night residual remains, scaled by solar brightness. Twilight is an
artistic approximation; ozone, multiple scattering, refraction, observer altitude,
weather and configurable planetary parameters are not modeled yet. A future
atmosphere profile can replace the internal terrestrial assumptions without
changing the same-node association or persisted solar units.

The solar disk uses an angular radius calculated in the shader from `solarSize`,
the fixed solar radius of 695700 km, and the configured distance.
A uniform disk's radiance is derived from effective normal irradiance using
`L = E / (pi * sin(angularRadius)^2)`. Disk radiance is filtered over the cubemap
pixel footprint with an approximate energy correction, so a small sun is not
simply discarded at the default 256-pixel face resolution. The disk is clipped at
the world horizon; the diffuse sky remains smooth around it. GLSL and Metal use
the same generation fields and formulas.

## Solar resource generation

`EnvironmentComponent::solarState()` resolves the association from its owner.
Detached component clones retain a runtime value snapshot, so queue extraction
and prepared editor environment states do not retain scene pointers. The solar
snapshot is not serialized. Adopting a clone into a node resolves that node's light.

`GenerationSettings::solar` accepts a runtime `base::SolarState` for direct
procedural-generation clients. Its default is disabled, preserving the old sky.
`includeSolarDisk` defaults to true. Generation uses a 128-byte push-constant
layout shared by C++, GLSL and Metal. Linked solar sky and IBL generation use
float32 HDR to accommodate the small disk's high radiance.

`EnvironmentGenerator::synchronize()` detects solar direction, size, color and
intensity changes as well as environment colors. It rebuilds sky and IBL at the
existing safe update boundary. Generation remains synchronous; interactive solar
rotation can therefore incur cubemap regeneration cost. There is no new background
scheduler or reduced-quality drag mode.

The sky cubemap and prefiltered specular retain the disk for reflection consumers.
The second procedural sky excludes the disk and is retained as
`EnvironmentResources::skyboxBackground`, together with the published `solar`
snapshot. An unblurred SkyboxRenderer samples this background and draws the disk
analytically at screen resolution using its physical angular radius and
screen-space derivative antialiasing, including horizon clipping. This prevents
the default 256-pixel cubemap from turning the small sun into a magnified square.
The skybox push layout is 112 bytes in C++, GLSL and Metal. A blurred skybox samples
the existing prefiltered specular image, including its filtered sun, without adding
a second analytic disk. Resources without an optional background retain the
original skybox sampling behavior.

Diffuse irradiance is built from the disk-free sky, because direct solar lighting
already comes from the directional light. The broad artistic sky transition stays
in diffuse IBL. This is an approximation, not a complete energy partition across
all direct and reflected transport paths. The BRDF LUT follows the existing
rebuild lifecycle. Generator state capture/exchange includes the resolved solar
values, preserving prepared environment undo/redo behavior. Failed regeneration
retains the last published environment resources and reports the existing error;
direct light extraction continues to use the current scene values.

## Solar gizmo asset

`GizmoType::SunLight` loads `assets/sun-light-gizmo.bg2`. It depicts a spherical sun,
a ring, twelve rays and a long arrow along local **+Z**, indicating emitted light
travel. The visible sun is in the opposite direction. The shape remains readable
without relying on material colors, which the existing unlit gizmo path overrides.
It has independent scale, opacity and visibility settings in the Gizmos UI.

Regenerate the asset with `python3 scripts/create_sun_light_gizmo.py`. The script
uses only the Python standard library and writes BG2 1.2: a four-byte version
header, `hedr`, JSON `mtrl`, empty `join`, named `plst` blocks, positions, normals,
two UV arrays, triangle indices and `endf`. Numeric arrays and lengths are big
endian. The two submeshes are `SolarBody` and `SolarEmissionDirection`; neither is
a transform handle. No BG2 loader or vendored library changes are needed.
