# ImageFormat

**Header:** `<bg2e/db/image.hpp>`

**Namespace:** `bg2e::db`

`ImageFormat` identifies the 8-bit image formats supported by the existing
stb-backed `saveImage` writer:

```cpp
enum class ImageFormat { PNG, JPEG, BMP, TGA };
```

## Format discovery

```cpp
auto format = db::imageFormatFromExtension("JPEG");
if (format) {
    auto extension = db::canonicalImageExtension(*format); // ".jpg"
}

auto pathFormat = db::imageFormatFromPath("bake.PNG"); // ImageFormat::PNG
```

`imageFormatFromExtension(std::string_view)` is case-insensitive and accepts
dotted or undotted extensions. Unknown extensions return `std::nullopt`.
`imageFormatFromPath(const std::filesystem::path&)` discovers the format from
the path's final extension.

## Extension mapping

| Format | Accepted extensions | Canonical extension |
|--------|---------------------|---------------------|
| `PNG` | `.png` | `.png` |
| `JPEG` | `.jpg`, `.jpeg` | `.jpg` |
| `BMP` | `.bmp` | `.bmp` |
| `TGA` | `.tga` | `.tga` |

`extensionsForImageFormat(format)` returns the accepted dotted extensions as
`std::vector<std::string_view>`. `canonicalImageExtension(format)` returns the
preferred dotted extension. For an invalid enum value, these helpers return an
empty list or empty view respectively.

## Saving images

The byte `saveImage` overloads dispatch using the path extension and retain
their writer behavior, including JPEG quality 100. Unsupported byte-image
extensions throw `std::runtime_error`.

```cpp
std::vector<uint8_t> rgb(width * height * 3);
db::saveImage(outputDirectory / ("chair" +
    std::string(db::canonicalImageExtension(*format))),
    rgb.data(), width, height, 3);
```

The float overload writes finite, nonnegative RGB/RGBA pixels to standard
Radiance `.hdr` (case-insensitive extension). RGBE preserves dynamic range with
quantization and discards alpha; exact float bits are not retained. It accepts
three or four components and rejects other extensions with
`std::invalid_argument`. HDR is separate from the 8-bit `ImageFormat` enum:
`imageFormatFromPath("map.hdr")` returns `std::nullopt`.

`loadImage(path)` selects floating-point HDR decoding for `.hdr` (case-insensitive)
and byte decoding for other supported images. Lightmap interpretation still
comes from the material's baked AO/GI settings; extension selects pixel precision,
not whether an image is a lightmap.

The GPU overload reads mip zero of a 2D RGBA8 UNORM or RGBA32F image. Use `.png`
for RGBA8 or `.hdr` for RGBA32F. Other GPU formats and cubemaps are rejected;
images require TransferSrc and callers must stop producers. Authored mip chains
are not stored. See [standard image export](FileIO.md#standard-image-export).

[`LightmapOutputWriter`](../bake/LightmapOutputWriter.md) writes already converted
RGB8 pixels to the selected byte format (PNG by default), or RGB32F pixels to
Radiance HDR. It does not apply a second exposure conversion.

Sources: [header](../../../lib/include/bg2e/db/image.hpp),
[implementation](../../../lib/src/bg2e/db/image_db.cpp).

## Function signatures

```cpp
std::optional<ImageFormat> imageFormatFromExtension(std::string_view extension);
std::optional<ImageFormat> imageFormatFromPath(const std::filesystem::path& filePath);
std::vector<std::string_view> extensionsForImageFormat(ImageFormat format);
std::string_view canonicalImageExtension(ImageFormat format);
void saveImage(const std::filesystem::path& filePath, const float* data,
               uint32_t width, uint32_t height, uint32_t channels);
void saveImage(const std::filesystem::path& filePath, gpu::Image& image);
void saveImage(const std::filesystem::path& filePath, const uint8_t* data,
               uint32_t width, uint32_t height, uint32_t bpp);
void saveImage(const std::filesystem::path& basePath, const std::string& fileName,
               const uint8_t* data, uint32_t width, uint32_t height, uint32_t bpp);
```

Here `bpp` is the number of byte components per pixel (use 3 for RGB), not a
bit count. Format discovery returns an optional; encoding failures and unsupported
save extensions throw. See [output architecture](../../architecture/lightmaps/output_and_materials.md).
