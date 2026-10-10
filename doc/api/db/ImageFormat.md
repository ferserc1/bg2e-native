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

The existing `saveImage` overloads dispatch using the path extension and retain
their writer behavior, including JPEG quality 100. Unsupported extensions
throw `std::runtime_error`.

```cpp
std::vector<uint8_t> rgb(width * height * 3);
db::saveImage(outputDirectory / ("chair" +
    std::string(db::canonicalImageExtension(*format))),
    rgb.data(), width, height, 3);
```

Float `LightmapPixels` are converted to clamped, rounded RGB8 by
[`LightmapOutputWriter`](LightmapOutputWriter.md); this API does not add HDR
file export.

Sources: [header](../../../lib/include/bg2e/db/image.hpp),
[implementation](../../../lib/src/bg2e/db/image_db.cpp).

## Function signatures

```cpp
std::optional<ImageFormat> imageFormatFromExtension(std::string_view extension);
std::optional<ImageFormat> imageFormatFromPath(const std::filesystem::path& filePath);
std::vector<std::string_view> extensionsForImageFormat(ImageFormat format);
std::string_view canonicalImageExtension(ImageFormat format);
void saveImage(const std::filesystem::path& filePath, const uint8_t* data,
               uint32_t width, uint32_t height, uint32_t bpp);
void saveImage(const std::filesystem::path& basePath, const std::string& fileName,
               const uint8_t* data, uint32_t width, uint32_t height, uint32_t bpp);
```

Here `bpp` is the number of byte components per pixel (use 3 for RGB), not a
bit count. Format discovery returns an optional; encoding failures and unsupported
save extensions throw. See [output architecture](../../architecture/lightmaps/output_and_materials.md).
