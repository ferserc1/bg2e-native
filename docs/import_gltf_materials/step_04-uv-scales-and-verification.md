# Step 04 — Import supported UV scales and verify round trips

## Changes

- For each glTF texture view, use `[1, 1]` by default. If `KHR_texture_transform` is present, map its two-component `transform.scale` to `albedoScale`, `metalnessScale`, `roughnessScale`, or `normalScale` according to the view. The shared metallic/roughness view supplies the same UV scale to both slots.
- Honor `transform.texcoord` when supplied; otherwise use the texture view's `texcoord`. Validate that it is 0 or 1 and present on the primitive.
- Treat glTF `normal_texture.scale` as **normal strength**, not UV scale. Leave it out of this change and report non-default values as unsupported. Likewise report nonzero UV offset or rotation: current material fields only express scale. Do not quietly reinterpret either value.
- Verify PNG paths survive the importer lifetime and remain readable until saving. Exercise scene save and drawable/model export followed by BG2 reload, including multiple materials sharing one image and one image used for both metallic and roughness.
- Check an alpha-bearing base-color PNG, a JPEG source converted to PNG, an opaque data map, external URI, data URI, and GLB buffer-view image. Check that distinct image entries with the same source basename do not collide inside one temporary import directory. Existing collisions when unrelated exports are copied to one BG2 destination remain out of scope.

## Completion check

The code compiles after this step. Supported texture UV scales and sets round-trip through BG2, image paths resolve after import and save, and unsupported transform/normal-strength cases produce an explicit diagnostic. No changes are made to `storeMeshBg2`, Vulkan readback, or CMake.
