# Draw Composer

The application target is `bg2e_composer`, implemented on `bg2e::draw`.

- [Application, deployment and lifecycle guide](../../doc/apps/draw_composer.md)
- [Guided scene workflow](../../doc/tutorials/draw/composer.md)
- [HTTP protocol](../../doc/apps/composer/import_service.md) and [draw import ownership](../../doc/apps/composer/draw_import_lifecycle.md)
- Lead checks: [scene IO](../../tests/draw/composer_io/README.md), [imports](../../tests/draw/composer_import/README.md), [integrated baking](../../tests/draw/composer_baking/README.md), [frame pacing](../../tests/draw/composer_frame_pacing/README.md)

Run `bg2e_composer [--backend vulkan|metal]` with deployed engine shaders/assets or its macOS bundle. Model Properties is on the left; Scene hierarchy/inspector is on the right. Environment/light/camera editing uses authored components. The UV/Lightmap window supports preview, selected-subtree and whole-scene integrated baking. Complete scene saves include actual GPU image resources. Standalone draw batch output remains future M08 work.
