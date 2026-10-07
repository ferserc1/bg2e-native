# Geometry API Reference

Public UV-atlas API declared by `<bg2e/geo/all.hpp>`:

| Symbol | Header | Documentation |
|--------|--------|---------------|
| `Uv2AtlasOptions`, `Uv2AtlasResult`, `GenerateUv2AtlasModifier` | `geo/GenerateUv2AtlasModifier.hpp` | [GenerateUv2AtlasModifier](GenerateUv2AtlasModifier.md) |
| `UvAtlasError`, `UvAtlasValidation`, `UvAtlasValidator` | `geo/UvAtlasValidator.hpp` | [UvAtlasValidator](UvAtlasValidator.md) |

These APIs operate on CPU mesh data. Atlas generation is transactional: a
failed `apply()` leaves the input mesh unchanged.
