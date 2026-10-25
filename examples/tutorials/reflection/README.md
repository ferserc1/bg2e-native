# Reflection tutorial application

Build target: `tutorial_reflection`, registered with the existing `bundle_app_sdl` helper.

Start with [Part 1: Scene and manipulation](../../../doc/tutorials/refection/01_scene_and_manipulation.md), then follow the links through the three-part tutorial. It explains how to reconstruct this application from `examples/draw/11_manipulation`.

The application retains that example's scene, picking, gizmos, camera controls, materials and graphics settings. It adds:

- A built-in NodeEditor window and an application-owned reflection editor, both targeting the first selected node even during multiple selection.
- A Rotation component with an X/Y/Z axis, speed in degrees per second and a Toggle rotation action that pauses/resumes without changing the configured speed. Original model has this component initially.
- An optional Reflection Samples component, available through Add Component, for all scalar property kinds, read-only views, nested objects, resources, polymorphic subtypes and callable actions. These sample values do not affect rendering.

Click to select; Ctrl-click adds/removes selections. Drag with the left button to orbit, right to pan, or middle to zoom. Use the existing Graphics settings button to open its initially closed window. The custom inspector queues edits for the next renderer update boundary; the built-in inspector retains the engine widget's existing editing behavior.

The tutorial components are excluded from scene serialization. No engine API or shader changes are required. Build and manual verification instructions are included in the tutorial; compilation and runtime validation must be performed separately.
