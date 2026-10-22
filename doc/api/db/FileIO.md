# Standard images and file publication

These `bg2e::db` helpers provide file/data operations. They do not own bake jobs,
complete scene exports or protected batch publication; those contracts belong to
[complete draw IO](DrawResourceIO.md) and the
[bake output writer](../bake/LightmapOutputWriter.md).

## Standard image export

Include `<bg2e/db/image.hpp>` or `<bg2e/db/all.hpp>`.
`saveImage(path, bytes, width, height, channels)` writes PNG/JPEG/BMP/TGA.
`saveImage(path, floats, width, height, channels)` writes linear RGB/RGBA
pixels as Radiance `.hdr`. RGBE preserves dynamic range with quantization;
it does not preserve exact floating-point bits or alpha.

`saveImage(path, gpuImage)` synchronously reads mip zero of a 2D RGBA8 UNORM
or RGBA32F image and writes PNG or HDR respectively. Other GPU formats and
cubemaps are rejected explicitly. Images require TransferSrc; callers must
stop producers. Mipmaps are regenerated on load according to texture settings.
No proprietary texture archive is written or loaded.

## publishFile

Include `<bg2e/db/file.hpp>` or `<bg2e/db/all.hpp>`.

```cpp
void publishFile(const std::filesystem::path& staged,
    const std::filesystem::path& destination, bool replaceExisting = false);
```

Publish a distinct regular nonsymlink staged file on the same filesystem.
The no-replace default is enforced by the OS against competing creators;
explicit replacement uses an atomic main-file switch. macOS uses exclusive
rename, Linux renameat2 NOREPLACE and Windows MoveFileExW; unsupported native
operations fail explicitly. The portable fallback needs a hardlink.

The helper creates no parent directories, exports no resources, performs no
whole-batch source/alias preflight and provides no power-loss durability. It
throws on failure. A product must retain staged inputs/previous generations and
publish its authoritative file only after every required resource is complete.
Do not delete an existing destination first to emulate replacement.

## Parsed OBJ dependency inventory

`db::ObjMesh::sourcePaths`, from `<bg2e/db/mesh_obj.hpp>`, records the original OBJ
and each successfully read MTL as absolute paths using the existing parser.
This CPU inventory does not mutate parsing/material behavior or discover custom
resources. The bake writer combines it with material/environment/explicit inputs
for its stronger protection contract.

Sources: [image declarations](../../../lib/include/bg2e/db/image.hpp),
[image codecs](../../../lib/src/bg2e/db/image_db.cpp),
[file publication](../../../lib/src/bg2e/db/file.cpp).
