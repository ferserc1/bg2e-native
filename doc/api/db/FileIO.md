# Generic native texture archives and file publication

These `bg2e::db` helpers provide file/data operations. They do not own bake jobs,
complete scene exports or protected batch publication; those contracts belong to
[complete draw IO](DrawResourceIO.md) and the
[bake output writer](../bake/LightmapOutputWriter.md).

## TextureArchive

Include `<bg2e/db/texture_archive.hpp>` or `<bg2e/db/all.hpp>`.
TextureArchive has `gpu::ImageDescription description` and
`vector<vector<uint8_t>> subresources` ordered **layer, then mip**.

```cpp
TextureArchive readTextureArchive(const std::filesystem::path& path);
void writeTextureArchive(const std::filesystem::path& path, gpu::Image& image);
void writeTextureArchive(const std::filesystem::path& path, const TextureArchive& archive);
```

The CPU overload creates no GPU resource. Supply explicit mipLevels and
minMipSize zero, supported native color format, valid 2D/cubemap extent and exact
native byte count for every layer/mip. Cubemaps have six layers. Invalid counts/
sizes reject before creating the output file. The GPU overload synchronously
reads native subresources; Device/image must remain alive with producers stopped.
Neither overload creates destination parents or protects arbitrary source paths;
callers stage/publicize files according to their product policy.

BG2TEX01 version 1 stores native format/type/width/height/mip count and length-
prefixed subresources. Current byte order support is little endian. Format bits,
HDR values and authored mips/layers are not gamma-converted or quantized. Reads
validate lengths/truncation/trailing bytes. The draw texture loader recognizes
`.bg2tex`; ordinary image codecs remain distinct from native texture archive decoding.
Native format enum interpretation follows the current engine archive contract.

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

Sources: [archive declarations](../../../lib/include/bg2e/db/texture_archive.hpp),
[archive encoding](../../../lib/src/bg2e/db/texture_archive.cpp),
[file publication](../../../lib/src/bg2e/db/file.cpp),
[OBJ reader](../../../lib/src/bg2e/db/mesh_obj_db.cpp).
