# Step 04: Add image-format discovery in db

## Scope

Introduce bg2e::db::ImageFormat and helpers mapping supported formats to extensions and filesystem paths/strings: PNG, JPEG (.jpg/.jpeg), BMP and TGA. Reuse the existing stb-backed saveImage implementation. Reject unsupported formats before baking. Keep HDR image export outside this plan; float CPU bake output remains available.

## Acceptance and compile gate

Format/extension mapping and current image saving compile and agree for all supported extensions. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Handoff

After this step, complete [prestep_05_headless_output.md](prestep_05_headless_output.md) for the next step (Define standalone output policy).
