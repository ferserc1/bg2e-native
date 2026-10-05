# Handoff for Add image-format discovery in db

Next implementation step: [step-04_image_formats.md](step-04_image_formats.md). Complete this file **after** finishing step 03; this template records no implementation results.

- Changed files and relevant API decisions: TODO
- Build command, platform and result: TODO
- Runtime/fixture evidence: TODO
- Remaining limitations or regressions: TODO
- Resources, ownership and synchronization cautions for the next agent: TODO
- Exact next action: Implement the scope in step-04_image_formats.md.

## Next-step instructions

Introduce bg2e::db::ImageFormat and helpers mapping supported formats to extensions and filesystem paths/strings: PNG, JPEG (.jpg/.jpeg), BMP and TGA. Reuse the existing stb-backed saveImage implementation. Reject unsupported formats before baking. Keep HDR image export outside this plan; float CPU bake output remains available.

## Next-step acceptance gate

Format/extension mapping and current image saving compile and agree for all supported extensions. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Add `enum class ImageFormat { PNG, JPEG, BMP, TGA }` in `bg2e::db` and functions `imageFormatFromExtension(std::string_view)`, `imageFormatFromPath(const std::filesystem::path&)`, `extensionsForImageFormat(ImageFormat)` and `canonicalImageExtension(ImageFormat)`. Return an optional format for unknown extensions; use case-insensitive matching and accept both dotted and undotted strings.
- Map `PNG -> .png`, `JPEG -> .jpg/.jpeg`, `BMP -> .bmp`, `TGA -> .tga`. The canonical JPEG extension is `.jpg`. Wire the existing `saveImage` dispatch to these helpers without changing its existing overloads or writer-quality behavior.
- Expose the enum/helpers from `lib/include/bg2e/db/image.hpp`; keep stb includes in implementation files. Verify both string and path forms, upper/lowercase, unsupported input and a real small image round-trip.
