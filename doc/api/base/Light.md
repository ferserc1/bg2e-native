# Light and LightData

**Header:** `<bg2e/base/Light.hpp>`  
**Namespace:** `bg2e::base`

`Light` describes illumination parameters without owning a scene transform.
Its property setters/getters are virtual.

| Getter / setter | Default |
|-----------------|---------|
| `color` / `setColor` | White. |
| `intensity` / `setIntensity` | 1. |
| `type` / `setType` | `TypeOmni`. |
| `spotAngle` / `setSpotAngle` | 22. |
| `spotCutoff` / `setSpotCutoff` | 14. |
| `castShadows` / `setCastShadows` | true. |
| `sourceSize` / `setSourceSize` | 0.5. |
| `shadowSamples` / `setShadowSamples` | 8. |
| `affectsReflections` / `setAffectsReflections` | true. |

`LightType` values are `TypeOmni = 5`, `TypeSpot = 1`,
`TypeDirectional = 4`, and `TypeDisabled = 10`.

## Serialization

`serialize()` writes the properties above as JSON. `typeString()` maps types to
`kTypePoint`, `kTypeSpot`, `kTypeDirectional`, or `kTypeDisabled`.
`deserialize(shared_ptr<json::JsonNode>)` updates present properties, ignores
invalid input objects, and maps unrecognized type strings to disabled.

## LightData

`LightData` is a plain transport structure containing world `position`,
`intensity`, `color`, `direction`, light `type`, `spotAngle`, `spotCutoff`,
integer `castShadows`, `sourceSize`, integer `shadowSamples`, integer
`affectsReflections`, and two padding fields. Populate all required fields
before using it; it has no property validation or resource ownership.

## See also

- [Base API](index.md)
- [API reference](reference.md)
