# Color

**Header:** `<bg2e/base/Color.hpp>`  
**Namespace:** `bg2e::base`

`Color` is an RGBA float value with public `r`, `g`, `b`, and `a` components.
Default construction sets every component to zero. `Color(r, g, b, a = 1.0f)`
constructs an explicit value. Components are not clamped.

```cpp
bg2e::base::Color background{0.12f, 0.18f, 0.28f, 1.0f};
auto white = bg2e::base::Color::White();
```

## Color space and named values

`Color::TypeSRGB` and `Color::TypeLinear` describe the interpretation used by
images and textures. A `Color` value itself has no stored color-space tag and
performs no automatic conversion.

Static factories provide `Black`, `White`, `Red`, `Green`, `Blue`, `Yellow`,
`Cyan`, `Magenta`, `Transparent`, `Gray`, `LightGray`, `DarkGray`, `Orange`,
`Purple`, `Pink`, `Brown`, `Gold`, `Silver`, `Bronze`, `Olive`, `Teal`, `Navy`,
`Coral`, `Salmon`, `Lavender`, `Peach`, `Mint`, `Indigo`, and `Violet`.
All named colors are opaque except `Transparent()`.

`toString()` returns `Color(r, g, b, a)` with each float multiplied by 255 and
converted to an integer; this is a display representation, not serialization.

## See also

- [Base API](index.md)
- [API reference](reference.md)
