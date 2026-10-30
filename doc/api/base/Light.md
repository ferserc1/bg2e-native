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
| `solarSize` / `setSolarSize` | 1.0 (one solar radius). |
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
`bg2e.draw.light.v1`) and `ui::LightEditor` expose **Sun**, **Solar Size**
and **Solar Distance (million km)**. Ordinary light type values remain unchanged.
Solar fields are stored for all light types and ignored by ordinary lights.
Missing solar JSON fields restore the defaults above. An old engine that does not
recognize `kTypeSun` will load it as disabled, following its existing unknown-type policy.

Solar size and distance must be finite and positive. Evaluation also requires
finite nonnegative RGB/intensity and a finite nonzero emission direction.
Shaders bound the radius-to-distance ratio to keep angular evaluation finite,
without changing authored parameters.

`intensity` remains the brightness control in existing engine units, calibrated
at 149.6 million km. There is no conversion to lux or watts. Effective intensity
scales by `(149.6 / solarDistanceMillionKm)^2`; the authored value is unchanged.
The emitter angular radius is `asin(solarSize * 695700 / (solarDistanceMillionKm * 1e6))`,
computed in GLSL and Metal using a fixed solar-radius constant.
It controls the procedural disk and overrides `sourceSize` in extracted solar
shadow calculations. The stored `sourceSize` is preserved for switching types.

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
world direction toward the sun, solar size multiplier, distance in millions of
kilometers, transmitted RGB, distance-scaled intensity and an atmosphere flag.
It is a value snapshot, with no component references or GPU ownership.

Atmospheric and horizon modifiers are never written to `Light::color()` or
`Light::intensity()`, and are not serialized. Draw extraction preserves `TypeSun`
(`6`) and sends the multiplier unchanged. Shared GLSL and Metal helpers derive
angular radius and horizon visibility before using the directional lighting path.
Environment shaders derive angular radius and disk radiance from the same inputs.
The light uniform layout is unchanged; a solar light occupies one of eight slots.

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
