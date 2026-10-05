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
