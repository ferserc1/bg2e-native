# Step 08 — Integrate EnvironmentComponent with the Resource picker

## Goal

Expose the environment map as a resource property and connect picker changes
to the existing texture cache and scene environment update flow.

## Files to modify or add

```text
lib/src/bg2e/scene/EnvironmentComponent.cpp
lib/src/bg2e/scene/reflection/EnvironmentComponentReflection.cpp
lib/include/bg2e/ui/ComponentInspector.hpp
lib/src/bg2e/ui/ComponentInspector.cpp
```

The exact application integration file depends on which editor owns the
`ComponentInspector`; do not introduce an engine dependency into reflection or
the generic picker.

## Component metadata

Register `environmentImage` as a Resource property with HDR/image filters.
Keep `imgHash` out of the editor because it is derived cache state.

The setter should continue to update the image path and hash. Do not make the
component's generic reflection setter load Vulkan resources directly.

## Change propagation

Define an explicit editor-side resource-change hook. When the selected path
changes, the owning editor/application should:

1. Resolve the selected path according to the scene resource convention.
2. Call `setEnvironmentImage`.
3. Load or refresh the image through `TextureCache` using the active engine.
4. Call `Scene::updateEnvironment()` when the scene reference changes.
5. Notify the existing inspector `onChanged` callback for dirty-state and save
   handling.

Avoid loading on every ImGui frame; perform the operation only after the
resource value changes. Handle missing files and cancelled dialogs without
destroying the previous environment.

## Serialization considerations

Keep the current copy-to-save-directory behavior in `serialize()`. The editor
must not rewrite scene files or copy assets merely because a picker is opened.

## Acceptance criteria

- The Environment component displays a Browse-enabled Resource field.
- Selecting an HDR/image path updates the component once.
- The texture cache and scene environment references are refreshed through the
  application context.
- Cancelled or invalid selections leave the previous environment intact.
- Existing serialization remains compatible.
- The project passes the build command in `README.md`.
