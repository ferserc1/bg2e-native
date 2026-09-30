# Step 01 — Map scalar material properties

## Changes

- In `lib/src/bg2e/db/scene_gltf.cpp`, create a helper that converts the scalar part of a `cgltf_material` to `base::MaterialAttributes`.
- For `has_pbr_metallic_roughness`, map `base_color_factor[4]` to albedo, `metallic_factor` to metalness, and `roughness_factor` to roughness. Use glTF defaults (white albedo, metallic 1, roughness 1) for missing material or missing metallic-roughness block, rather than `MaterialAttributes`' different defaults.
- Preserve the existing submesh name behavior. Set the converted material on the corresponding drawable submesh after `setMesh` and before `load(engine)`. Match by mesh index and primitive index; do not rely on the global `submeshNames` cursor for material lookup.
- Keep the helper limited to the core metallic-roughness model. Other glTF material extensions can retain current defaults or receive an explicit unsupported warning.

## Completion check

The code is compilable with no image helper or texture binding present. A glTF with color and numeric factors but no textures produces those factors on the matching drawable submeshes. An absent material gets glTF defaults. Multi-primitive meshes retain the correct per-primitive material.
