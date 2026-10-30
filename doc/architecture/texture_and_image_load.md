# Texture and Image Loading

## Scope

This document describes the current file-based image and texture loading paths,
including explicit loading, caching, model and scene imports, and environment
images. The current APIs discussed here realize resources through `bg2e::draw`.

The fundamental distinction is:

```text
File -> base::Image -> draw::Texture -> GPU image + sampler
           CPU              GPU

base::Texture describes the source and sampling settings; it does not read files.
```

## 1. Source descriptions: base::Texture

Header: [`Texture.hpp`](../../lib/include/bg2e/base/Texture.hpp).

`base::Texture` can describe a filesystem source through:

- A `std::filesystem::path` constructor.
- A `std::string` constructor.
- A directory-plus-filename constructor.
- `setImageFilePath()`.

These operations store a path and select `TypeFilesystem`. They do not decode
pixels, validate that the file exists, or allocate GPU resources. Relative paths
remain relative until a caller resolves them or the file is opened.

The same class can describe generated pixels through `TypeProcedural` and a
`ProceduralTextureGenerator`. That is an alternative source, not a file loader.

### Sampling and color defaults

| Setting | Default |
| --- | --- |
| Mipmaps | Enabled |
| Magnification filter | Linear |
| Minification filter | Nearest |
| Mip LOD bias | 0 |
| Minimum LOD | 0 |
| Maximum LOD | -1, meaning the maximum available mip count |
| U/V/W addressing | Repeat |
| Color interpretation | sRGB |

`revision()` tracks changes to authored settings and observes procedural generator
revisions. It does not track changes to filesystem contents.

See [Texture and ProceduralTextureGenerator](../api/base/Texture.md).

## 2. CPU image decoding: db::loadImage

Header: [`image.hpp`](../../lib/include/bg2e/db/image.hpp).

```cpp
base::Image* loadImage(const std::filesystem::path& filePath);
base::Image* loadImage(const std::filesystem::path& basePath,
                      const std::string& fileName);
```

These functions open and decode a file into CPU pixels. They do not require a
graphics context and do not allocate a GPU texture. The directory/name overload
joins its arguments and delegates to the path overload.

The returned `base::Image*` belongs to the caller. Use an owning wrapper:

```cpp
std::unique_ptr<bg2e::base::Image> pixels(
    bg2e::db::loadImage(std::filesystem::path("textures/albedo.png")));
```

The implementation records the source path in the image and always requests four
channels from stb:

| File | Decoder | Output |
| --- | --- | --- |
| `.hdr`, case-insensitive | `stbi_loadf` | RGBA floating-point pixels |
| Other stb-compatible images | `stbi_load` | RGBA 8-bit pixels |

Decoded stb allocations are copied into buffers owned by `base::Image`. Failed
decoding throws `std::runtime_error` with the source path.

### Reading formats versus writing formats

`ImageFormat { PNG, JPEG, BMP, TGA }` describes the supported 8-bit writer formats
and their extension helpers. It is not an exhaustive declaration of image reader
formats. Reading delegates to stb, with the explicit `.hdr` floating-point branch
described above.

Radiance HDR is separate from that enum: `imageFormatFromPath("map.hdr")` returns
`std::nullopt`, even though `loadImage()` supports HDR.

### Decoding encoded memory

`decodeImageRGBA8()` accepts encoded image bytes and returns a `std::vector<uint8_t>`
containing four-channel 8-bit pixels, with width and height output parameters. It
does not open a file or upload a GPU resource. The glTF importer uses this boundary
for image payloads obtained from files, data URIs, or buffer views.

Sources: [`image_db.cpp`](../../lib/src/bg2e/db/image_db.cpp),
[ImageFormat documentation](../api/db/ImageFormat.md).

## 3. Direct GPU realization: draw::Texture

Header: [`draw/Texture.hpp`](../../lib/include/bg2e/draw/Texture.hpp).

### Loading from source settings

```cpp
auto settings = std::make_shared<bg2e::base::Texture>(
    std::filesystem::path("textures/albedo.png"));

auto texture = std::make_shared<bg2e::draw::Texture>(&engine, settings);
```

Alternatively, construct `draw::Texture` with the Engine and subsequently call
`load(settings)`.

For a filesystem source, the implementation follows this path:

```text
base::Texture settings
  -> clone editable settings
  -> db::loadImage(settings.imageFilePath())
  -> create GPU image
  -> upload pixels
  -> optionally generate mipmaps
  -> create sampler
  -> publish new image, sampler, and settings
```

Direct construction does not consult `TextureCache`.

### Loading previously decoded pixels

```cpp
bg2e::draw::Texture texture(&engine);
texture.load(*pixels, *settings);
```

This separates file decoding from GPU realization. The pixel overload also accepts
default texture settings. When settings describe an empty filesystem path, the
texture retains a CPU pixel copy for later reloads.

### GPU formats and mipmaps

The pixel-loading path selects:

- `R8G8B8A8_UNORM` for byte pixels.
- `R32G32B32A32_SFLOAT` for floating-point pixels.

If mipmaps are enabled, it allocates a full chain down to minimum size one and
generates the mip levels. Unsupported mipmap generation or linear filtering raises
an error rather than silently changing the requested behavior. Color interpretation
is retained in texture settings; it is distinct from selecting byte versus float
storage.

For matching four-channel byte RGBA8 UNORM and float RGBA32F data, GPU
`uploadImage()` now forwards the source directly to the backend upload path. It
does not allocate the generic conversion buffer or convert each component through
float. Other layouts retain conversion. Backend staging still copies the pixels;
this is not a zero-copy GPU allocation or an asynchronous upload API.

### Reloading and resource ownership

`reload()` reloads retained CPU pixels when present; otherwise it loads the current
source settings again. For a file-backed texture, this reopens and decodes the
file. There is no automatic filesystem watcher.

`updateSampler()` changes sampling settings without reading the source file.

The overload accepting settings and an existing `gpu::Image` adopts a compatible
GPU image and creates its sampler; it is not itself a file-loading operation.

Replaced image and sampler owners are deferred through the Engine cleanup manager.
The Engine and device are borrowed and must outlive textures and their GPU users.

Implementation: [`draw/Texture.cpp`](../../lib/src/bg2e/draw/Texture.cpp).

## 4. Explicit shared loading: utils::TextureCache

Header: [`TextureCache.hpp`](../../lib/include/bg2e/utils/TextureCache.hpp).

The singleton exposes four loading forms:

```cpp
load(engine, filesystemPath);
load(engine, directory, filename);
load(engine, stringPath);
load(engine, textureSettings);
```

Each returns a loaded `std::shared_ptr<draw::Texture>`:

```cpp
auto texture = bg2e::utils::TextureCache::get().load(
    &engine, std::filesystem::path("textures/albedo.png"));
```

The settings overload requires a non-null Engine, `TypeFilesystem`, and a nonempty
source path. Procedural sources are rejected by this cache.

### Cache identity

The key includes:

- Device identity.
- Absolute, lexically normalized source path.
- Minification and magnification filters.
- Mipmap enablement, LOD limits, and LOD bias.
- U/V/W address modes.
- Color interpretation.

Before reuse, the implementation also checks the originating Engine, live image
validity, image device, and the cached texture's current settings.

Consequences:

- The same file with different settings can produce separate GPU textures.
- Lexical normalization is not filesystem canonicalization: symbolic-link aliases
  are not necessarily deduplicated.
- File modification times and content hashes are not checked. A cache hit does
  not detect edited image contents.
- Entries are weak. External owners, not the cache, retain texture lifetime.
- Expired or incompatible entries are replaced on a subsequent load.
- `emptyCache()` drops weak entries without destroying externally owned textures.
- `destroy()` releases the singleton.

### Default-filter difference

Path and string overloads construct settings and explicitly select linear
minification. The `base::Texture` overload preserves supplied settings, whose
initial minification filter is nearest. These calls can therefore produce distinct
cache entries even for the same source path.

Sources: [`TextureCache.cpp`](../../lib/src/bg2e/utils/TextureCache.cpp),
[TextureCache documentation](../api/utils/TextureCache.md).

## 5. Material realization

Model importers generally create `base::Texture` descriptions first. Actual image
decoding and GPU creation occur when the material is realized.

`draw::MaterialBase::updateTextures()` chooses between:

```text
Filesystem source + texture cache enabled -> TextureCache::load()
Filesystem source, cache disabled         -> ImageLoadContext::load()
Procedural source                         -> independent draw::Texture
Unassigned material slot                  -> context-shared neutral image
```

The `_useTextureCache` setting defaults to **false**. Model import therefore does
not imply automatic sharing through `TextureCache`.

### Import-scoped image reuse

[`draw::ImageLoadContext`](../api/draw/ImageLoadContext.md) reuses image resources
without sharing mutable texture objects. Existing `Drawable::load()` and
`updateMaterials()` calls create a local context shared by all their material
slots. Standalone `MaterialBase::updateTextures()` creates one for that operation.
Overloads accept an explicit context to extend reuse across Drawables; glTF/GLB
uses one context for its import.

The context is bound to one Engine and strongly retains:

- Decoded CPU pixels keyed by absolute, lexically normalized file path.
- GPU images keyed by that path, mipmap enablement, and color interpretation.
- One white and one flat-normal image for unassigned slots.

Sampler settings do not affect image identity. Every realized texture has its own
settings and sampler, so addressing/filter edits remain independent. A packed
metallic/roughness image can be decoded, uploaded, and mipmapped once for both
slots; AO can share it too when its image identity is compatible. BG2 image paths
repeated across materials benefit in the same way.

Context destruction releases its CPU and image-cache owners; returned textures
retain their images. Keep contexts short-lived because decoded pixels remain
resident for the entire context scope. Reload/load replace a texture's image rather
than modifying shared pixels. Raw GPU image access must treat shared storage as
read-only; deep-clone before independently writing pixels.

There is no content hashing, timestamp invalidation, persistent cache, or
cross-device reuse. A new context observes current file contents. Procedural
sources bypass reuse. Explicit `useTextureCache()` still selects the existing
whole-texture cache instead of context image reuse. Callbacks retain per-slot
semantics, not unique-image or physical-upload counts.

Material realization tracks settings owners and revisions to avoid redundant
uploads. Unassigned slots receive runtime placeholders; a failed assigned image
load is not equivalent to an unassigned slot.

Sources: [`MaterialBase.cpp`](../../lib/src/bg2e/draw/MaterialBase.cpp),
[`MaterialBase.hpp`](../../lib/include/bg2e/draw/MaterialBase.hpp).

## 6. Loading textures through models

### BG2 and VWGLB model resources

The BG2 reader deserializes material descriptions through `MaterialSerializer`.
Texture filenames are joined to the model's base directory. The serializer's
initial texture settings use linear minification and magnification with mipmaps
enabled, before applying serialized settings.

- `loadMeshBg2()` prepares CPU geometry, material attributes, and image references.
- `loadDrawableBg2(..., engine, ...)` realizes geometry and material resources.
- `loadSceneBg2(..., engine, ...)` builds the model Node and realizes its resources.

Scene Drawable references accept `.bg2` and `.vwglb` through the BG2 resource path.
The material images are separate file resources resolved from model metadata.
Engine-aware loading uses a common image context for the Drawable, so repeated
paths with compatible image settings reuse their decoded pixels and GPU image.

Sources: [`mesh_bg2.cpp`](../../lib/src/bg2e/db/mesh_bg2.cpp),
[`MaterialSerializer.cpp`](../../lib/src/bg2e/utils/MaterialSerializer.cpp).

### OBJ and MTL

`readObj()` prepares CPU geometry and MTL material descriptions.

Path resolution has two levels:

```text
OBJ directory -> referenced MTL file
MTL directory -> referenced image file
```

Texture references resolve relative to their corresponding MTL, not necessarily
the OBJ directory. `loadDrawableObj()` realizes these descriptions with the supplied
Engine. Overloads accept either a path or the result of `readObj()`.

Imported maps populate the six material slots: albedo, metalness, roughness,
normal, ambient occlusion, and emission. Relevant names, UV scales, and clamp
addressing are retained. A missing MTL is an error. Separate opacity maps,
height-to-normal conversion, and texture offsets/turbulence require conversion
before import and raise errors.

Source: [`mesh_obj_db.cpp`](../../lib/src/bg2e/db/mesh_obj_db.cpp).

### glTF and GLB

`loadGltf()` supports image payloads from:

1. External image files, resolved relative to the glTF file after URI decoding.
2. Base64 `data:` URIs.
3. Image `bufferView` data, including embedded GLB image payloads.

Remote image URIs containing `://` are rejected. Data URIs must use the supported
base64 form.

#### External image paths

External URI paths are decoded and resolved relative to the model, then retained
as absolute, lexically normalized filesystem sources. They are not converted to
temporary PNGs. The shared import context decodes each source path once as needed.

#### Embedded image materialization and direct pixel reuse

For data URIs and buffer views, `TemporaryImages` preserves a PNG reload/export
source but seeds the import context with the pixels it already decoded:

```text
Data URI / buffer view
  -> encoded bytes
  -> db::decodeImageRGBA8()
  -> temporary PNG file for later reload/export
  -> register decoded RGBA8 pixels under that PNG path
  -> base::Texture pointing to the PNG
  -> ImageLoadContext: upload registered pixels once per image configuration
  -> independent texture settings/samplers referencing the shared GPU image
```

The temporary path map reuses a materialized file for the same glTF image index
within the import. The context spans all imported Drawables and additionally
reuses compatible GPU image storage across slots and materials. Initial realization
does not read or decode the generated PNG again. The conversion produces RGBA8
pixels, not a floating-point HDR image path. On successful import, temporary files
remain available as texture sources; an uncommitted temporary image directory is
removed by its owner. Later reloads use the PNG through ordinary file loading.

PNG encoding/writing is deliberately retained to preserve the existing source
contract. Avoiding that work as well would require retained encoded/pixel sources
and deferred filesystem materialization on export.

The importer transfers PBR texture references, color interpretations, supported UV
sets, and sampler settings. It is a static-model importer, not a general glTF
transcoder; unsupported scene and material features can require prior conversion.

`readGltf()` prepares an owning parser document and loaded CPU buffers without
creating graph or GPU resources. Image decoding and realization still happen in
`loadGltf(document, engine, ...)`. External images must remain available: the CPU
document is not an immutable snapshot of those files.

Source: [`scene_gltf.cpp`](../../lib/src/bg2e/db/scene_gltf.cpp).

## 7. Loading textures through scene documents

Header: [`scene.hpp`](../../lib/include/bg2e/db/scene.hpp).

| API | Behavior |
| --- | --- |
| `readSceneDocument()` | Reads JSON without building a graph or opening referenced resources |
| `loadScene()` without an Engine | Builds a CPU graph with unresolved resource references |
| `loadScene()` with `draw::Engine&` | Builds the graph and invokes resource realization |

Engine-aware scene loading calls component resource hooks. Drawable components
resolve BG2/VWGLB and OBJ resources, whose material realization loads their images.
A glTF hierarchy must be imported with `loadGltf()` first; it is not represented
by one Drawable resource reference.

Scene reference serialization and raw JSON IO do not materialize or copy image
pixels. Complete resource saves do: assigned filesystem images are copied, while
supported assigned GPU-only or procedural images can be exported as standard
PNG/HDR mip zero. Keep scene/model files and their sidecar resources together.

Proprietary texture archives are no longer supported and require external
conversion to standard images.

See [Complete draw resource IO](../api/db/DrawResourceIO.md).

## 8. Environment image loading

`EnvironmentGenerator::build()` provides another file-loading
path:

```text
Equirectangular source image
  -> db::loadImage()
  -> environment source GPU image
  -> cubemap and IBL resource generation
```

This path does not use `TextureCache`. Although its source parameter is named
`hdrSource`, the implementation accepts readable byte images as well as
floating-point HDR images. It validates image data and device capabilities before
building environment resources. An empty source selects generated sky behavior
rather than loading a file.

Environment source references are also represented by scene environment
components.

Sources:
[`EnvironmentGenerator.cpp`](../../lib/src/bg2e/draw/environment/EnvironmentGenerator.cpp),
[`EnvironmentComponent.cpp`](../../lib/src/bg2e/scene/EnvironmentComponent.cpp).

## 9. Synchronization and lifetime requirements

- CPU image decoding does not itself allocate GPU resources.
- GPU uploads and high-level resource realization are synchronous.
- Use an initialized Engine for `draw::Texture`, engine-aware model/scene imports,
  and environment generation.
- Stop frame recording and background submissions before resource realization or
  binding replacement. Publish edits before frame acquisition, with no unsubmitted
  recording borrowing old bindings.
- Keep the Engine/device alive until all loaded textures and GPU users are released.
- Coordinate cache access and changes with stopped producers; the cache is not a
  background resource-loading service.
- CPU model readers can prepare independent inputs on owned workers, but their
  results must be synchronized before stopped-thread realization. They do not
  automatically snapshot external images or perform background GPU loading.

## 10. Choosing a loading path

| Requirement | API |
| --- | --- |
| Describe a file and its sampling settings | `base::Texture` |
| Decode file pixels without GPU resources | `db::loadImage()` |
| Decode an encoded memory payload | `db::decodeImageRGBA8()` |
| Create an independent GPU texture from a file | `draw::Texture` with source settings |
| Upload already decoded pixels | `draw::Texture::load(base::Image, settings)` |
| Explicitly share compatible file-backed GPU textures | `TextureCache::load()` |
| Reuse CPU decoding and GPU images with independent texture objects during an import | `draw::ImageLoadContext` |
| Load model geometry and material images | Engine-aware BG2, OBJ, or glTF import |
| Load a graph and its referenced resources | `loadScene(..., engine, ...)` |
| Load an equirectangular environment and generate IBL | `EnvironmentGenerator::build()` |

## 11. Future improvement: batched GPU uploads and mipmaps

GPU batching is not implemented by these optimizations. Current backend uploads
still synchronize outstanding users, allocate staging, submit immediately, and
wait for completion. Mipmap generation uses a separate synchronous submission.

A future explicit upload-batch API could retain staging/resources for several
images, record copies and mip generation together, and wait at the import
publication boundary rather than per operation. It would need completion-aware
resource ownership, layout/dependency handling, rollback, and equivalent Vulkan
and Metal behavior. Existing synchronous convenience APIs should retain their
contracts; simply deleting waits would not be safe.

Staging pools, parallel CPU decoding, content-hash deduplication, stb allocation
adoption, and deferred embedded-image PNG writing are also outside the current
scope.

This document describes source contracts and implementation paths. It does not
claim compilation, backend execution, performance measurements, or runtime validation.
