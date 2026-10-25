# MaterialAttributes

**Header:** `<bg2e/base/MaterialAttributes.hpp>`  
**Namespace:** `bg2e::base`

`MaterialAttributes` stores CPU material parameters, texture descriptions, and
mesh-related metadata. GPU realization is managed by higher-level consumers.

## Scalar and color attributes

Every property below has a getter with the listed name and a `set`-prefixed
setter (for example, `albedo()` / `setAlbedo(...)`).

| Property | Default / meaning |
|----------|-------------------|
| `isTransparent` | false; transparency selection. Setter: `setIsTransparent`. |
| `isSolid` | true. Setter: `setIsSolid`. |
| `isUnlit` | false. Setter: `setIsUnlit`. |
| `refractionFactor` | 0.017; screen-space refraction strength for transparent materials. |
| `alphaCutoff` | 0; discard below final albedo alpha threshold. |
| `albedo` | White; setters accept `Color` or float arrays of length 3/4. |
| `metalness` | 0. |
| `roughness` | 0.9. |
| `fresnelTint` | White. |
| `sheenIntensity`, `sheenColor` | 0 and White. |
| `lightEmission` | 0. |
| `name`, `groupName`, `visible` | Empty names and true; metadata retained for mesh serialization. |

## Texture slots

Slots are `albedoTexture`, `metalnessTexture`, `roughnessTexture`,
`normalTexture`, `aoTexture`, and `lightEmissionTexture`. Each getter returns
`shared_ptr<Texture>`; setters accept shared pointers or raw pointers that
transfer ownership. All slots initially have no texture.

Each slot has a scale and UV-set pair using its prefix, for example
`albedoScale()` / `setAlbedoScale(...)` and `albedoUVSet()` /
`setAlbedoUVSet(...)`. Scales default to `(1, 1)` and accept GLM vectors or
float arrays; UV-set indices default to zero. Metalness, roughness, AO, and
emission expose channel selection (default 0). Metalness, roughness, and
emission also expose inversion (default false).

## Indirect texture interpretation

`indirectTextureMode()` / `setIndirectTextureMode(...)` selects
`IndirectTextureMode::AmbientOcclusion`, `BakedAmbientOcclusion`, or
`BakedGlobalIllumination`. The default is `AmbientOcclusion`; unsupported values
throw `std::invalid_argument`.

The AO slot, `aoScale`, and `aoUVSet` can carry a baked RGB indirect multiplier.
Baked modes bypass scalar AO. Baked GI affects diffuse IBL only; baked AO
affects all IBL.

## Change tracking

`revision()` tracks changes to authored state independently of texture realization.
Texture setters mark the associated `*TextureUpdated()` flag false.
`setUpdated()` marks all texture flags true; `isUpdated()` requires all six.
Copy construction is defaulted; assignment tracks copied attribute changes.

## See also

- [Base API](index.md)
- [API reference](reference.md)
