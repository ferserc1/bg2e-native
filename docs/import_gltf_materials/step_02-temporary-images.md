# Step 02 — Create and cache temporary PNG images

## Changes

- Add an import-local image resolver in `scene_gltf.cpp`, or a small helper under `lib/src/bg2e/db/` with a matching header under `lib/include/bg2e/db/` if the logic becomes unwieldy. Its input is the glTF file path, parsed `cgltf_data`, and a `cgltf_image*`; its output is an absolute path to a PNG.
- Create one uniquely named directory under `std::filesystem::temp_directory_path()` per import. Generate one filename per glTF image index, for example `image_0003.png`; do not use the source image basename as the temporary identity.
- Resolve all three core image forms: external URI relative to the glTF file, data URI decoded from base64, and GLB `buffer_view` bytes via `cgltf_buffer_view_data` and `buffer_view->size`. Decode the encoded image bytes to RGBA8. `cgltf_load_buffers` loads buffer data, but does not decode PNG/JPEG image payloads. Use the project's existing stb image decoder, avoiding a second implementation macro in another translation unit.
- Validate missing bytes, unsupported image encodings, invalid dimensions, overflow, and failed PNG writes. Use `db::saveImage(path, rgbaBytes, width, height, 4)` and return an error with the glTF image index and source file context.
- Cache successful paths by `cgltf_image*` or its index. On repeated requests return the same path without decoding or writing again. Never key the cache by URI text: separate image entries may have equal names but different bytes.
- Keep generated files after the importer returns. On a failed import, clean up only files created by that failed import; do not remove files already handed to a successfully returned drawable.

## Completion check

The helper is callable and compilable independently of material texture wiring. A focused check covers external PNG/JPEG, embedded GLB image, data URI, and two texture references to one image yielding one path. Every output is a readable PNG with RGBA8 pixels. The project can compile at this point even though normal rendering still uses only scalar materials.
