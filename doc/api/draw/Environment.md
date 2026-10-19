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
