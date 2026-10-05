# Step 04: Add image-format discovery in db

## Scope

Introduce bg2e::db::ImageFormat and helpers mapping supported formats to extensions and filesystem paths/strings: PNG, JPEG (.jpg/.jpeg), BMP and TGA. Reuse the existing stb-backed saveImage implementation. Reject unsupported formats before baking. Keep HDR image export outside this plan; float CPU bake output remains available.

## Acceptance and compile gate

Format/extension mapping and current image saving compile and agree for all supported extensions. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Concrete deliverables

- Add `enum class ImageFormat { PNG, JPEG, BMP, TGA }` in `bg2e::db` and functions `imageFormatFromExtension(std::string_view)`, `imageFormatFromPath(const std::filesystem::path&)`, `extensionsForImageFormat(ImageFormat)` and `canonicalImageExtension(ImageFormat)`. Return an optional format for unknown extensions; use case-insensitive matching and accept both dotted and undotted strings.
- Map `PNG -> .png`, `JPEG -> .jpg/.jpeg`, `BMP -> .bmp`, `TGA -> .tga`. The canonical JPEG extension is `.jpg`. Wire the existing `saveImage` dispatch to these helpers without changing its existing overloads or writer-quality behavior.
- Expose the enum/helpers from `lib/include/bg2e/db/image.hpp`; keep stb includes in implementation files. Verify both string and path forms, upper/lowercase, unsupported input and a real small image round-trip.

## API example

```cpp
auto fmt = db::imageFormatFromPath("bake.JPEG"); // ImageFormat::JPEG
auto ext = db::canonicalImageExtension(*fmt);    // ".jpg"
```

## Handoff

After this step, complete [prestep_05_headless_output.md](prestep_05_headless_output.md) for the next step (Define standalone output policy).
