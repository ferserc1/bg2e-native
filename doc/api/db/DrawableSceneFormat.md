# Drawable scene format

This reference describes the drawable records written to scene JSON, how to
import them into the TypeScript SDK, and how to resolve the BG2 models and
material images they reference. The JSON keys and enum values are case-sensitive.

## Scene tree and component dispatch

The document is a JSON object with a required `scene` array of top-level node
objects. The writer emits `fileType: "bg2e::scene"` and
`version: { "major": 1, "minor": 0, "rev": 0 }`. The native reader requires the
array, allows an absent `fileType`, rejects any other supplied `fileType`,
and treats the version as informational. It attaches every top-level node
to a synthetic root. A malformed top-level entry fails the document import.
Traverse each node's `children` recursively. Each node's `components` array
contains component objects. Both arrays are optional on input. The reader skips
malformed child and component entries within a node. A
drawable instance is a component with `type: "DrawableInstance"`. Its node
participates in the normal hierarchy, with placement supplied by the existing
`Transform` component. All nodes use the same scene directory for resource
resolution. Preserve node settings such as `name`, `enabled` and `steady`
through the existing scene importer.

Both drawable component types write `priority` as a JSON number. Read it as
an unsigned 32-bit integer (0 through 4294967295). An absent or invalid
value leaves the default of 0 on a newly constructed component.

### Drawable owner

| Key | JSON type | Import behavior |
| --- | --- | --- |
| `type` | string literal `"Drawable"` | Dispatch to the existing model owner importer. |
| `priority` | uint32 | Common component priority. |
| `path` | nonempty string | BG2 model location. Resolve relative paths against the scene directory. An empty string is invalid. |
| `resourceName` | string, optional | Display name for the loaded drawable. Use the BG2 filename stem when absent. |
| `name` | nonempty string, legacy input | If `path` cannot be read as a string, resolve this value against the scene directory and replace its extension with `.bg2`. A readable but empty `path` fails before this fallback. |

The current owner writer emits `type`, `priority`, `path` and, when available,
`resourceName`. Model geometry and owner material definitions are in the BG2.
Repeated legacy owners of the same path are separate logical owners with
independent edits. Instances referring to that path bind to the first shared
resource for the path. Keep the original model definition available for
instance imports.

### Drawable instance

| Key | JSON type | Import behavior |
| --- | --- | --- |
| `type` | string literal `"DrawableInstance"` | Dispatch to the instance importer. |
| `priority` | uint32 | Common component priority. |
| `path` | nonempty string | Resolve relative to the scene directory and load the referenced BG2. A missing, empty or malformed value leaves the instance unresolved with a diagnostic. |
| `rayTracingEnabled` | boolean | Authored tracing eligibility. When absent or invalid, retain the resource's tracing flag. Effective tracing also depends on drawable eligibility in the renderer. |
| `submeshes` | array of objects | Array position is the zero-based BG2 submesh index. When absent or malformed, retain the resource's original submesh records. |

Each submesh object uses these keys.

| Key | JSON type | Import behavior |
| --- | --- | --- |
| `material` | object | Overlay the material fields described below on a copy of the original BG2 material at this index. |
| `transform` | matrix | Local transform for this submesh. Use the original submesh transform when absent or invalid. |
| `name` | string | Submesh display name. Use the original BG2 polygon-list name when absent or invalid. |
| `groupName` | string | Submesh group. Use the original group when absent or invalid. |
| `visible` | boolean | Submesh visibility. Use the original visibility when absent or invalid. |

Submesh names and groups are metadata. The importer matches records by index.
An absent material or malformed material object leaves the inherited material
intact. An invalid array element leaves the corresponding original submesh
intact. When the lengths differ, use the geometry's submesh count, warn, ignore
surplus records and keep the originals at indices with no record. Apply the same
index rule when reconciling a resource with a changed submesh count.

A submesh matrix is a flat JSON array of exactly 16 finite numbers in column-major order
(`m[col * 4 + row]`). Translation occupies indices 12, 13 and 14. The last row
(indices 3, 7, 11 and 15) must be `[0, 0, 0, 1]`. The absolute determinant must
be at least `1e-12` and finite. With column vectors, use
`nodeWorld * localSubmeshTransform * position`. Read the node's placement
from its sibling component with `type: "Transform"` and the column-major
`transformMatrix` field. The transform reader accepts any array of exactly
16 finite numbers and uses identity on a missing or malformed field. A node
without a Transform component contributes identity to the parent-to-child
world matrix product. A freshly loaded BG2 has identity submesh transforms.
Use identity submesh transforms and tracing enabled as provisional defaults while
the model is unresolved. Reapply retained submesh JSON over the model defaults on binding.

## Material object

The same material vocabulary is used in the BG2's embedded JSON material array
and in a scene instance's `material` object. Instance material records are
independent of the owner and other instances. The name, group and visibility
inside a material are separate fields from those on the enclosing instance
submesh object. The native render queue requires both submesh and material
visibility for a submesh to render.

| Keys | JSON type and interpretation |
| --- | --- |
| `type`, `class` | Strings emitted as `"pbr"` and `"PBRMaterial"`. The native material reader does not use them for dispatch. |
| `name`, `groupName` | Strings. |
| `isTransparent`, `isSolid`, `visible`, `isUnlit`, `doubleSided` | Booleans. The reader also accepts the legacy boolean `unlit` if it cannot read `isUnlit` as a boolean. |
| `refractionFactor`, `alphaCutoff`, `metalness`, `roughness`, `sheenIntensity`, `lightEmission` | Finite JSON numbers. |
| `albedo` | Four finite numbers `[r,g,b,a]` on write. The reader also accepts `[r,g,b]` and sets alpha to 1. |
| `fresnelTint`, `sheenColor` | Four finite numbers `[r,g,b,a]`. |
| `indirectTextureMode` | String enum `"ao"`, `"bakedAO"`, `"bakedGI"` for ambient occlusion, baked ambient occlusion and baked global illumination. |

All keys in an input instance material are optional. Apply valid supplied
values over the original values. Ignore unknown keys when building typed state.
Warn for malformed recognized fields and treat them as absent. Use actual JSON
types. Numeric strings and numeric booleans are invalid. Use the existing BG2
material decoder for the model material. Each BG2 material starts with empty
name and group, `isSolid` and `visible` true, and other booleans false.
`refractionFactor` defaults to 0.017, `roughness` to 0.9, and the other scalar
material values to 0. The three color fields default to opaque white.
`indirectTextureMode` starts at `"ao"`. Texture slots start unassigned,
with scale `[1,1]`, UV set and channel 0, and inversion false. As a legacy
BG2 read rule, an assigned `ambientOcclussion` texture with no
`ambientOcclussionUV` uses UV1. The instance overlay instead preserves the
inherited UV set for an omitted field. Unknown indirect modes or out-of-range
integer sampler enums can fail BG2 material decoding. The instance overlay
validates those fields individually and falls back to inherited or new-file
defaults.

### Texture slots and UV controls

Each row gives the exact source key, settings key and supported controls. A dash
indicates the absence of that control in the serialized format.

| Source key | Settings key | Scale | UV set | Channel | Inversion |
| --- | --- | --- | --- | --- | --- |
| `albedoTexture` | `albedoTextureSettings` | `albedoScale` | `albedoUV` | — | — |
| `metalnessTexture` | `metalnessTextureSettings` | `metalnessScale` | `metalnessUV` | `metalnessChannel` | `metalnessInvert` |
| `roughnessTexture` | `roughnessTextureSettings` | `roughnessScale` | `roughnessUV` | `roughnessChannel` | `roughnessInvert` |
| `normalTexture` | `normalTextureSettings` | `normalScale` | `normalUV` | — | — |
| `ambientOcclussion` | `ambientOcclussionSettings` | `ambientOcclussionScale` | `ambientOcclussionUV` | `ambientOcclussionChannel` | — |
| `lightEmissionTexture` | `lightEmissionTextureSettings` | `lightEmissionScale` | `lightEmissionUV` | `lightEmissionChannel` | `lightEmissionInvert` |

Keep the historical spelling `ambientOcclussion`, including in the related
keys. Scale is an array of two finite numbers `[u,v]`. The instance overlay
accepts UV set integers 0 and 1, channel integers 0 (R), 1 (G), 2 (B) and 3 (A),
and boolean inversion flags. Apply valid controls even when the slot has no
texture. An omitted or invalid control leaves its inherited value in an
instance, including when the source of the texture changes.

For each source key in an instance, distinguish three cases before applying
settings.

1. **Absent** preserves the inherited texture. An empty string or any other
   invalid value emits a warning and is handled as absent.
2. **JSON null** clears the texture. Any accompanying settings have no texture
   to modify. UV controls still apply.
3. **Nonempty string** replaces the texture with a file reference resolved
   against the scene directory. Initialize the settings from the new-file
   defaults below, even when the path refers to the same image as the original.

A settings object without a source key modifies a copy of the inherited
texture's settings when there is an inherited texture. Missing or invalid
fields fall back to the settings of the selected texture. An absent settings
object leaves those settings alone. For ordinary BG2 material definitions,
source keys are file strings relative to the BG2 directory when present and
typically omitted for unassigned slots. The instance's explicit clearing
behavior is specific to its overlay.

| Settings field | JSON type and values | New file default |
| --- | --- | --- |
| `mag`, `min` | Integer 0 = linear, 1 = nearest | 0 |
| `u`, `v`, `w` | Integer 0 = repeat, 1 = mirrored repeat, 2 = clamp to edge, 3 = clamp to border | 0 |
| `color` | Integer 1 = sRGB, 2 = linear | 1 |
| `mipmaps` | Boolean | `true` |
| `minLod` | Finite number | 0 |
| `maxLod` | Finite number. `-1` selects the available mip range. | -1 |
| `bias` | Finite number, mip LOD bias | 0 |

Resolve `indirectTextureMode` before restoring the AO texture. When the
material decoder creates or restores that slot from a file or settings
record, it forces linear interpretation for `bakedAO` and `bakedGI`, even
if the settings say sRGB. A mode-only overlay does not rewrite an existing
texture's authored settings. Preserve RGB data for baked GI. The channel
record still round-trips.

## Import sequence and resource resolution

1. Parse the node tree and dispatch components by their exact `type` values.
   Retain unresolved instance appearance data and the scene directory.
2. Resolve the model path. An instance can load its BG2 even when no scene
   node owns the model. Use a scene-scoped cache keyed by normalized absolute
   model path, or by the equivalent normalized resource URL for a URL-based
   SDK. Reuse the geometry and the original material definitions. Separate
   independently editable owner and instance appearance from those originals.
3. Decode the BG2 in polygon-list order. The embedded material data is a JSON
   array. For each polygon list, resolve its material by the exact material
   name. Select the first unused matching array entry, falling back to the
   first matching entry when all matches have been used. A polygon list without
   a matching material is an error. The native decoder takes submesh names from
   polygon lists and initializes submesh groups and visibility from the resolved
   material. The native mesh reader flips each stored UV's V coordinate and
   uses UV0 for UV1 when the second array is empty. Apply these conventions once
   in the BG2 decoder.
4. Start each instance from a copy of the model's original appearance and
   apply its component and submesh overlays. Resolve each relative image
   reference against its **declaring document**. Inherited BG2 texture paths
   stay resolved against the BG2 directory. Instance texture paths resolve
   against the scene directory. Absolute references retain their own location.
   Keep the decoded image shareable where appropriate, and keep texture
   settings and material state independently editable.
5. Realize the effective appearance and attach it to the node at its existing
   position in the hierarchy. If the model cannot load, retain the component,
   its reference and authored properties for a retry. If required instance
   material resources fail to prepare, retain authored state and suppress
   rendering of the affected instance. Continue importing other valid nodes
   for a recoverable drawable resource failure. Report which reference or
   field failed.

For dependency inventories, follow both edges. Collect each instance's explicit
file-valued material slots from the scene and expand every referenced BG2 into
its own image references. A `null` slot creates no scene-to-image edge, but the
BG2 can still depend on its original image. Settings objects carry no file paths.

### Worked example

```text
project/
  scene.json
  models/
    chair.bg2
    chair-base.png
    chair-normal.png
  lighting/
    chair-west.hdr
```

```json
{
  "fileType": "bg2e::scene",
  "version": {
    "major": 1,
    "minor": 0,
    "rev": 0
  },
  "scene": [
    {
      "type": "Node",
      "name": "Example",
      "enabled": true,
      "steady": false,
      "components": [],
      "children": [
        {
          "type": "Node",
          "name": "West chair",
          "enabled": true,
          "steady": false,
          "components": [
            {
              "type": "Transform",
              "priority": 0,
              "transformMatrix": [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 4, 0, 0, 1]
            },
            {
              "type": "DrawableInstance",
              "priority": 0,
              "path": "models/chair.bg2",
              "rayTracingEnabled": true,
              "submeshes": [
                {
                  "name": "Seat",
                  "groupName": "Furniture",
                  "visible": true,
                  "transform": [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1],
                  "material": {
                    "normalTexture": null,
                    "indirectTextureMode": "bakedGI",
                    "ambientOcclussion": "lighting/chair-west.hdr",
                    "ambientOcclussionUV": 1,
                    "ambientOcclussionSettings": {
                      "color": 2
                    }
                  }
                }
              ]
            }
          ],
          "children": []
        }
      ]
    }
  ]
}
```

Suppose the first polygon list of `chair.bg2` names the material `SeatSource`
and the BG2 material array contains the following entry.

```json
{
  "name": "SeatSource",
  "type": "pbr",
  "class": "PBRMaterial",
  "albedo": [1, 1, 1, 1],
  "albedoTexture": "chair-base.png",
  "normalTexture": "chair-normal.png"
}
```

The instance inherits `models/chair-base.png`, clears its normal assignment,
and uses `lighting/chair-west.hdr` from the scene directory as a linear baked
GI texture on UV1. Its submesh name becomes `Seat` and its material name stays
`SeatSource`. The model's own dependency on `models/chair-normal.png` remains.

## What the writer emits

- A resolved instance writes the common component fields, model path, tracing flag
  and one complete submesh record per geometry index. The node's placement
  is serialized by its `Transform` component.
  It writes a material object with the scalar fields, UV controls and explicit
  `null` for unassigned texture slots. Assigned slots have a file string and
  a settings object. Cache identifiers and resource residency are runtime state.
- Reference-only saving rebases paths against the target scene directory and
  copies no files. When lexical relative conversion is unavailable, the writer
  uses an absolute path. Unresolved instances can retain partial appearance and
  missing file references. A resolved instance with an assigned source that
  cannot be described by a filesystem path, including a source marked to skip
  serialization, requires the complete save route.
- Complete saving exports one BG2 per used identity, using the owner's
  properties when present or original definitions otherwise. Model export
  bakes the drawable and submesh transforms into BG2 vertices. With
  `B = exportedDrawableTransform * exportedLocalSubmeshTransform` for an
  index, the writer stores `instanceLocalSubmeshTransform * inverse(B)` in
  submesh record at `submeshes[i].transform`. A fresh importer applies this
  stored transform directly to the published BG2 geometry. When the native
  loader reuses resident pre-export geometry, it multiplies a valid saved
  submesh transform on the right by the recorded basis for that model path.
  An omitted or invalid submesh transform retains the resident default. That
  cache basis is runtime data.
- Complete saving copies filesystem images, materializes procedural sources as
  PNG, and can read back suitable current 2D GPU images at mip zero as PNG or
  Radiance HDR. It omits sources marked to skip serialization, which appear as
  cleared slots in the instance JSON. The scene packager deduplicates copies by
  canonical source path and reserves destination names. BG2 material strings
  use filenames relative to the published model. Scene material strings address
  the published images relative to the scene. Images contain pixels. Sampler,
  color interpretation and UV controls live in the material records.
- The scene file is published last. A complete save can report recoverable
  warnings when it preserves existing usable references. If it cannot retain a
  complete required reference, the save fails without publishing the new
  document.

The native implementation is in
[scene document IO](../../../lib/src/bg2e/db/scene.cpp),
[instance component IO](../../../lib/src/bg2e/scene/DrawableInstanceComponent.cpp),
[material IO](../../../lib/src/bg2e/utils/MaterialSerializer.cpp),
[BG2 model IO](../../../lib/src/bg2e/db/mesh_bg2.cpp), and
[complete draw resource IO](../../../lib/src/bg2e/db/draw_resource_io.cpp).

See [scene reference IO](SceneReferences.md), [complete resource IO](DrawResourceIO.md),
[dependency analysis](SceneResourceManager.md), and
[drawable cache architecture](../../architecture/drawable_caches_and_instances.md).
