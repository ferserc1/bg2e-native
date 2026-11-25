# Draw Composer

The application target is `bg2e_composer`, implemented on `bg2e::draw`.

- [Application, deployment and lifecycle guide](../../doc/apps/draw_composer.md)
- [Toolbar customization and Window menu](../../doc/apps/draw_composer.md#toolbar-and-window-menu)
- [Guided scene workflow](../../doc/tutorials/draw/composer.md)
- [HTTP protocol](../../doc/apps/composer/import_service.md) and [draw import ownership](../../doc/apps/composer/draw_import_lifecycle.md)
- Lead checks: [scene IO](../../tests/draw/composer_io/README.md), [imports](../../tests/draw/composer_import/README.md), [integrated baking](../../tests/draw/composer_baking/README.md), [frame pacing](../../tests/draw/composer_frame_pacing/README.md)

Run `bg2e_composer [--backend vulkan|metal]` with deployed engine shaders/assets or its macOS bundle. Model Properties is on the left; Scene hierarchy/inspector is on the right. Environment/light/camera editing uses authored components. The UV/Lightmap window supports preview, selected-subtree and whole-scene integrated baking. Complete scene saves include actual GPU image resources. Standalone draw batch output remains future M08 work.

## Persistent GPU diagnostics

Use `--deep-debug` with `--backend metal` or `--backend vulkan` to enable a bounded
persistent trace in Debug or Release. The log is written beside the executable;
copy it before restarting, since a new session replaces the previous log.
See [deep debug logging](../../plans/deep-debug-tools/README.md) for file ownership,
retention, failure behavior and diagnostic limitations.
