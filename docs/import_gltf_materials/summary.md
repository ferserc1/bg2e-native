# glTF material import plan

## Goal

Import glTF 2.0 metallic-roughness materials into `bg2e::scene::Drawable` submeshes: base color and texture, metallic and roughness factors and texture, normal texture, UV sets, and texture UV scales. Decode every referenced source image to one PNG in a dedicated temporary import directory. Reuse that PNG wherever the same glTF image is referenced. The resulting file paths allow the existing renderer to load textures and the existing BG2 exporter to copy them when saving a model or scene.

## Implementation order

1. [Map scalar material properties](step_01-material-factors.md).
2. [Create and cache temporary PNG images](step_02-temporary-images.md).
3. [Connect glTF texture views to submesh materials](step_03-texture-bindings.md).
4. [Import supported UV scales and verify round trips](step_04-uv-scales-and-verification.md).

Complete each step as an independent, compilable change before starting the next. This document is a plan only; no source changes or builds are part of writing it. During implementation, compile at each step if the implementer has permission to build. The project instructions prohibit compiling without an explicit request.

## Shared decisions and limits

- Keep `storeMeshBg2`, the BG2 material serializer, and their existing basename-based image sharing behavior unchanged. Existing filename collisions between unrelated textures remain a documented exporter limitation.
- Generate PNGs for all referenced images, including opaque albedo. PNG preserves alpha and the numerical values in metallic/roughness and normal maps. Use `bg2e::db::saveImage` after decoding to RGBA8; do not rename encoded JPEG bytes to `.png`.
- Cache by `cgltf_image*` (or image index), not by material, texture view, URI, or filename. Distinct glTF images get distinct temporary filenames. If two texture views refer to one image, they get one PNG path while retaining their own sampler and UV settings.
- Create an import-specific directory under `std::filesystem::temp_directory_path()` with a unique name. Store absolute paths in `base::Texture`. Do not remove PNGs when `loadGltf` returns: drawables and the texture cache may outlive the parser, and saving can occur later. Document eventual temporary-file cleanup as an operational concern; automatic cleanup is outside this change.
- Keep material assignment before `Drawable::load(engine)`, because loading creates render materials and reads their image files.
- Do not add Vulkan readback or a central texture manager. No CMake changes are needed; code belongs in existing source files or auto-discovered source locations.
- Scope normal `scale` separately from UV scale: glTF normal strength scales decoded normal X/Y, whereas `MaterialAttributes::normalScale` scales UVs. Do not write glTF normal strength into `normalScale`. Full normal-strength support would require a separate material/shader change.
- `KHR_texture_transform` offset and rotation have no equivalent in the current material API. The plan imports its scale and effective UV set only. If an asset uses unsupported transform terms or UV sets above 1, report the limitation rather than silently pretending the material matches.

## Existing path

`scene_gltf.cpp` currently gives each glTF primitive one submesh and reads only its material name. `Drawable::setMesh` creates the matching material slots. `Drawable::load` creates render materials and loads textures. `storeMeshBg2` serializes texture basenames and copies their source files beside the `.bg2` output; scene serialization reaches this path through `DrawableComponent`. The temporary PNGs provide those source files without changes to the exporter.
