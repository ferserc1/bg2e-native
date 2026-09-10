# Material modifier gallery

Build the `material_modifier` target and run `bin/linux/material_modifier`
(or the corresponding platform executable/app bundle). The example uses the
engine's bundled rust textures and Gothic Manor environment; no downloads are
needed. It uses the production deferred renderer.

Left-drag to orbit, right-drag to pan, and use the wheel to zoom. The camera
starts with the whole gallery in view. The control window can be moved or
resized; scroll it to reach the remaining scenarios.

## Scene and scenarios

The grid is **one drawable with 36 submeshes**. Roughness increases from left to
right and metalness from bottom to top. Each row belongs to `row-0` through
`row-5`. The bottom two rows start blue: the same modifier constructed from a
JSON node is applied before and after loading. The input node is changed to red
between construction and application, demonstrating that the modifier owns a
snapshot.

Four larger spheres sit below the grid, from left to right:

1. Rust material applied to CPU attributes before loading.
2. The same material applied to a loaded drawable, including all six texture
   slots. UV set 1 has twice the coordinates of UV set 0. The roughness map is
   intentionally reused as an AO demonstration, not as a physically accurate AO map.
3. A purple surface for direct reference/shared-pointer `MaterialBase` edits.
4. Transparent glass with refraction and `isSolid: false`.

The control window exercises:

| Control | Expected result |
|---|---|
| Single submesh | Submesh 14 becomes cyan; the others retain their colors. |
| Exact group | Six spheres in row 2 become green. |
| Regex | Twelve spheres in rows 3 and 5 become gold. |
| All submeshes | All 36 spheres become blue; the metalness/roughness gradient remains. |
| Apply rust | Runtime texture loading, six callback invocations, same appearance as the left reference sphere. |
| Scale / UV / channels only | Existing textures remain assigned while independent companion fields change. |
| Serializer JSON | The second sphere receives the first sphere's serialized properties, with absolute paths resolved independently of the supplied base path. |
| MaterialBase reference / shared pointer | The third large sphere changes immediately through both live-material overloads. |
| Explicit zero / false | Emission, sheen, metalness and unlit can be disabled explicitly. |
| Update materials + reload | Grid overrides persist through CPU-to-render synchronization and rebuilding. |
| Preserve unrelated runtime edits | A direct runtime roughness edit survives a drawable-level albedo-only patch. |
| Invalid input / unmatched groups | Malformed JSON, unsupported class, invalid index and null targets are rejected; invalid fields, metadata and unmatched groups leave the grid unchanged. |

Status is displayed in the window and printed to the console. These are
interactive integration scenarios; no unit-test target or framework is added.

## API contract

`MaterialModifier` snapshots an object supplied as a `std::string` or a
`std::shared_ptr<bg2e::json::JsonNode>`. Values are optional, including booleans
and zeroes. Unknown keys, nulls, wrong types, empty texture paths, invalid
channel indices (outside 0–3) and invalid UV indices (outside 0–1) are ignored.
Texture removal through null or an empty path is not supported.

The string reader rejects malformed documents as a whole. A private strict
reader produces native JSON nodes because the existing general-purpose engine
parser tolerates some malformed separators and partial results. Nesting is
limited to 128 levels and numbers must fit finite native floats.

The canonical keys follow `MaterialSerializer`, including `unlit` and the
historical `ambientOcclussion` spelling. `metalnessInvert` and
`roughnessInvert` are supported native attributes that the current serializer
does not yet emit/read. The serializer itself is unchanged.

CPU-only application does not load GPU textures. Live-material and loaded
drawable application refresh textures using the existing engine/cache policy;
call them on the render thread at the same safe point as the material editor.
Texture-loading exceptions propagate and updates are not transactional.

Drawable application patches its CPU and live material copies separately,
preserving unspecified properties in each. It does not reconcile pre-existing
differences between these copies. A later `updateMaterials()` can still replace
unrelated edits made directly to `MaterialBase`, as in the existing engine.
Use the drawable overload for overrides that must survive reloads.

Bulk overloads return the number of successful applications, including valid
no-op patches. Regex selection uses `regex_search`, matching the TypeScript
`RegExp.test` behavior. Names, groups and visibility are submesh metadata and
are never changed by material overrides.
