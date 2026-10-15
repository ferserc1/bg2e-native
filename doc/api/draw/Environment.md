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
