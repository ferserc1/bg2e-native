# Handoff for Add image-format discovery in db

Next implementation step: [step-04_image_formats.md](step-04_image_formats.md). This handoff records step 03 implementation findings; compile and prefab fixture verification remain pending.

- Changed files and relevant API decisions: Added `render::StandaloneBakeSceneAssembler` in `lib/include/bg2e/render/StandaloneBakeSceneAssembler.hpp` and `lib/src/bg2e/render/StandaloneBakeSceneAssembler.cpp`, exported by `bg2e/render/all.hpp`. Its `assembleModel` and `assemblePrefab` methods return one context `scene::Scene` plus deterministic target records that keep the input source path separate from each output identity. It accepts either a standard scene JSON document or a serialized Node subtree for prefabs. Context and target inputs are preflighted for malformed JSON, missing registered components, missing .bg2 files, and referenced texture files before DB loading.
- Build command, platform and result: Not run by the agent; repository instructions prohibit compiling unless explicitly requested. Project-lead compile verification for standalone step 03 is pending.
- Runtime/fixture evidence: Not run. The planned two-module sofa fixture should confirm both modules are attached to the same returned context scene, appear as deterministic target records, and participate in the single TLAS so module A can occlude module B.
- Remaining limitations or regressions: The existing DB loaders upload Drawables during import. Optional UV2 generation therefore modifies only the loaded in-memory CPU meshes after `device().waitIdle()` and reloads each affected Drawable to rebuild raster resources and BLAS before returning. No source JSON, .bg2, or texture file is written.
- Resources, ownership and synchronization cautions for the next agent: `Assembly::contextSourcePath`, `Assembly::inputSourcePath` (model .bg2 or prefab JSON), each `Target::inputSourcePath` (.bg2 backing that Drawable), and `Target::outputIdentity` are independent values. Output planning should derive names from the deterministic identity and never overwrite any input source. Prefab hierarchy and local transforms are retained under the context root. UV2 options are optional; when supplied they are passed to one `GenerateUv2AtlasModifier` per target Drawable, covering all of its submeshes. `StandaloneBakerContext::updateScene()` must run only after the returned scene has all targets attached.
- Exact next action: Implement `step-04_image_formats.md`, adding `db::ImageFormat` discovery and extension helpers for PNG, JPEG, BMP, and TGA without changing current image-writer overloads.

## Next-step instructions

Introduce bg2e::db::ImageFormat and helpers mapping supported formats to extensions and filesystem paths/strings: PNG, JPEG (.jpg/.jpeg), BMP and TGA. Reuse the existing stb-backed saveImage implementation. Reject unsupported formats before baking. Keep HDR image export outside this plan; float CPU bake output remains available.

## Next-step acceptance gate

Format/extension mapping and current image saving compile and agree for all supported extensions. Compile the project at the end of this step when implementation is authorized. Do not weaken an existing public API. Review changed files and describe any remaining runtime risk.

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Add `enum class ImageFormat { PNG, JPEG, BMP, TGA }` in `bg2e::db` and functions `imageFormatFromExtension(std::string_view)`, `imageFormatFromPath(const std::filesystem::path&)`, `extensionsForImageFormat(ImageFormat)` and `canonicalImageExtension(ImageFormat)`. Return an optional format for unknown extensions; use case-insensitive matching and accept both dotted and undotted strings.
- Map `PNG -> .png`, `JPEG -> .jpg/.jpeg`, `BMP -> .bmp`, `TGA -> .tga`. The canonical JPEG extension is `.jpg`. Wire the existing `saveImage` dispatch to these helpers without changing its existing overloads or writer-quality behavior.
- Expose the enum/helpers from `lib/include/bg2e/db/image.hpp`; keep stb includes in implementation files. Verify both string and path forms, upper/lowercase, unsupported input and a real small image round-trip.
