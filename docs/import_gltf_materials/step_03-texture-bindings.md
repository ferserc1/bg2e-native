# Step 03 — Connect texture views to submesh materials

## Changes

- Extend the Step 01 material conversion helper to accept the Step 02 image resolver.
- Map `pbr_metallic_roughness.base_color_texture` to albedo, `pbr_metallic_roughness.metallic_roughness_texture` to both metalness and roughness, and `material.normal_texture` to normal. Only create `base::Texture` objects when the corresponding `cgltf_texture_view.texture` and `texture->image` are valid.
- Set metalness channel to blue (`2`) and roughness channel to green (`1`); leave inversion off. Keep one generated PNG for their shared `cgltf_image`.
- Apply each texture view's `texcoord` to the matching material UV-set field, supporting 0 and 1. Validate the chosen primitive actually has the required UV attribute; the current geometry importer copies UV0 into UV1 when UV1 is absent, which would conceal an unsupported reference. Define an explicit diagnostic or failure for out-of-range or missing UV sets.
- Preserve the texture image's role: albedo is color data, metallic/roughness and normal maps are linear data. Check the existing render texture/color-conversion path and set `base::Texture` color type accordingly; do not alter packed texture pixel values when generating PNGs.
- Where a glTF texture has a sampler, map supported min/mag filtering and wrap modes onto its `base::Texture`; otherwise use glTF defaults. Separate `base::Texture` objects may point to the same PNG when views need different settings. Check the current path-based render `TextureCache`, which may reuse the first sampler for a repeated path, and handle that limitation without creating a second PNG for the same image.
- Assign all material data before `Drawable::load(engine)` so the image files are available when render materials are built.

## Completion check

This step compiles and renders textured glTF assets. One packed metallic/roughness image produces one temporary PNG and two material slots with different channel selections. The importer can save the drawable as BG2 and reload it using the unchanged source-file copy behavior. If sampler differences cannot be represented by the current path-based render cache, document that limitation explicitly before claiming sampler parity.
