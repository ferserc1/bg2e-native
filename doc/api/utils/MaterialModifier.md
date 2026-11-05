# MaterialModifier

**Header:** `<bg2e/utils/MaterialModifier.hpp>`  
**Namespace:** `bg2e::utils`

`MaterialModifier` parses a JSON material description into a reusable partial
PBR patch. Missing, null or incorrectly typed values are ignored; submesh
metadata is not changed. It patches `base::MaterialAttributes`,
`draw::MaterialBase` and `draw::Drawable` targets. Production overloads are removed.

```cpp
utils::MaterialModifier patch(jsonText, basePath);
if (patch.isValid())
{
    patch.apply(*drawable, submeshIndex);
    // Request redraw/history reset through the owning application/delegate.
}
```

Drawable overloads patch stored CPU material definitions and live material
attributes independently. They do not copy a whole material or call
`updateMaterials()`, which could overwrite unrelated runtime edits. Direct
`MaterialBase` overloads modify only that live material and do not synchronize
an owning drawable.

`applyAll(drawable, groupName)` targets an exact group name;
`applyAll(drawable, regex)` uses `std::regex_search`. The return value counts
successfully selected/applied submeshes (including valid no-op patches), or
zero for an invalid patch/no matches.

`bool changesTextureSource(uint32_t slot) const noexcept` is read-only patch
inspection for supported editor source locks. Slots 0–5 are albedo, metalness,
roughness, normal, AO and emission. An invalid patch returns false; an invalid
slot warns/returns false. It reports source assignment intent, not sampling/UV
changes. Generic direct `apply` does not enforce procedural ownership; editor
mutation owners preflight all receivers and reject locked source changes before
publishing anything. `doubleSided` is part of ordinary partial material patches.

For draw targets, call at a stopped scene/GPU boundary. Texture load/update
exceptions propagate, and a multi-submesh patch is not transactional: earlier
patches may have applied. The caller should request redraw and reset relevant
render histories after edits, including partial failure. The utility does not
own application invalidation policy. See the
[draw material editor](../ui/Material_Editors.md#drawmaterialeditor).
