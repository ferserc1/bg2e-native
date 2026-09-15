# Step 03 — Add generic Resource metadata and a file picker

## Goal

Provide a generic editor for filesystem-backed resources without coupling the
reflection layer to Vulkan or a specific texture cache. The first consumer is
the EnvironmentComponent image path.

## Files to modify or add

```text
lib/include/bg2e/reflection/Property.hpp
lib/include/bg2e/reflection/Builder.hpp
lib/include/bg2e/ui/ReflectionWidget.hpp
lib/src/bg2e/ui/ReflectionWidget.cpp
lib/include/bg2e/ui/ResourcePicker.hpp
lib/src/bg2e/ui/ResourcePicker.cpp
```

The exact picker filename may follow the repository's UI naming convention,
but it must remain a generic UI component and not depend on `scene` or
`render::Engine`.

## Reflection metadata

`PropertyType::Resource` already exists. Add the minimum metadata needed for a
picker:

- Resource kind or filter identifier.
- Optional accepted extensions.
- Whether the stored value is absolute or project-relative.

Prefer a small, serializable metadata representation rather than embedding a
callback or engine pointer in `PropertyInfo`. The generic reflection layer
must remain independent of `app`, `render`, and `scene`.

Add a builder method such as `resource(...)` or `resourceFilter(...)` that
configures this metadata without changing ordinary `String` and `Path`
properties.

## UI behavior

Implement a `ResourcePicker` using existing primitives and
`app::FileDialog`:

- Show the current path or resource name.
- Provide a Browse button.
- Apply the configured extension filters.
- Return the selected path and a changed flag.
- Handle cancel without modifying the property.
- Respect read-only properties by disabling both text editing and Browse.

`ReflectionWidget` should dispatch `PropertyType::Resource` to this widget.
The picker only edits metadata values; it must not load textures itself.

## Acceptance criteria

- A reflected Resource property can be edited and returns a changed flag.
- Cancelled dialogs leave the old value untouched.
- Generic UI code does not include `scene`, `render::Engine`, or
  `utils::TextureCache`.
- Existing String, Path, and TextureWidgets behavior is unchanged.
- The project passes the build command in `README.md`.
