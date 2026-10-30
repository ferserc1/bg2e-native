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
| `solarRadiusKm` / `setSolarRadiusKm` | 695700 km. |
| `solarDistanceMillionKm` / `setSolarDistanceMillionKm` | 149.6 million km. |

`LightType` values are `TypeOmni = 5`, `TypeSpot = 1`,
`TypeDirectional = 4`, `TypeSun = 6`, and `TypeDisabled = 10`.

## Serialization

`serialize()` writes the properties above as JSON. `typeString()` maps types to
`kTypePoint`, `kTypeSpot`, `kTypeDirectional`, `kTypeSun`, or `kTypeDisabled`.
`deserialize(shared_ptr<json::JsonNode>)` updates present properties, ignores
invalid input objects, and maps unrecognized type strings to disabled.

## Solar lights

Select `TypeSun` on the existing `LightComponent`; no new component or polymorphic
light storage is required. Both reflection registrations (`bg2e::base::Light` and
`bg2e.draw.light.v1`) and `ui::LightEditor` expose **Sun**, **Solar Radius (km)**
and **Solar Distance (million km)**. Ordinary light type values remain unchanged.
Solar fields are stored for all light types and ignored by ordinary lights.
Missing solar JSON fields restore the defaults above. An old engine that does not
recognize `kTypeSun` will load it as disabled, following its existing unknown-type policy.

Solar radius and distance must be finite and positive. Evaluation additionally
requires distance in km to exceed radius, finite nonnegative RGB/intensity and a
finite nonzero emission direction. Invalid combinations report an error instead
of silently changing authored parameters.

`intensity` remains the brightness control in existing engine units, calibrated
at 149.6 million km. There is no conversion to lux or watts. Effective intensity
scales by `(149.6 / solarDistanceMillionKm)^2`; the authored value is unchanged.
The emitter angular radius is `asin(solarRadiusKm / (solarDistanceMillionKm * 1e6))`.
It controls the procedural disk and overrides `sourceSize` in extracted solar
shadow data, in degrees. The stored `sourceSize` is preserved for switching types.

With a procedural Environment on the same active node, the sun receives
terrestrial atmospheric RGB attenuation and horizon visibility. The environment
and draw light extraction use the same derived solar model. Without that active
association, including when HDR is selected, the sun renders as a directional
emitter with its physical angular size and distance scaling, without atmospheric
attenuation or horizon clipping. Only the active Environment drives a sky.

The node's world-transformed local `+Z` is light propagation; its opposite points
toward the visible sun. Translation places the editor gizmo but does not move a
sun across the sky. World `+Y` defines the horizon. Ancestor transforms contribute
to the direction, which is normalized before evaluation.

### Runtime solar values

`<bg2e/base/SolarLight.hpp>` exposes `SolarState` and
`evaluateSolarLight(light, rayDirection, atmosphere = true)`. The state contains
world direction toward the sun, angular radius in radians, transmitted RGB,
effective direct intensity, disk radiance and the sky intensity scale.
It is a value snapshot, with no component references or GPU ownership.

Atmospheric and horizon modifiers are never written to `Light::color()` or
`Light::intensity()`, and are not serialized. Draw extraction maps `TypeSun` to
the existing directional GPU type (`4`), preserving the light uniform layout and
existing raster, ray tracing and standalone baking consumers. A solar light
occupies one of the existing eight light slots.

See [procedural environment and solar association](../draw/Environment.md#solar-association).

## LightData

`LightData` is a plain transport structure containing world `position`,
`intensity`, `color`, `direction`, light `type`, `spotAngle`, `spotCutoff`,
integer `castShadows`, `sourceSize`, integer `shadowSamples`, integer
`affectsReflections`, and two padding fields. Populate all required fields
before using it; it has no property validation or resource ownership.

## See also

- [Base API](index.md)
- [API reference](reference.md)
