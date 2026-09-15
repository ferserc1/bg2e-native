# Step 05 — Render polymorphic objects in `ReflectionWidget`

## Goal

Consume the metadata from Step 04 and render a subtype selector plus the
active subtype's reflected properties.

## Files to modify

```text
lib/include/bg2e/ui/ReflectionWidget.hpp
lib/src/bg2e/ui/ReflectionWidget.cpp
```

## Implementation

Add a protected `drawPolymorphicObjectProperty` path alongside the existing
scalar and by-value object paths.

The UI flow is:

1. Resolve the active subtype metadata from the reflection registry.
2. Draw a Combo with the registered subtype labels.
3. If the selection changes, call the replacement callback.
4. Re-read the current object after replacement; never retain a pointer across
   the replacement call.
5. Draw the active subtype's properties and actions recursively.

Use the existing object-depth limit for recursive subtype property drawing.
The widget must preserve disabled/read-only behavior when no replacement or
mutable object path is available.

If a subtype is not registered, show a disabled diagnostic and do not invoke
its accessors. If the current object is null, show the subtype selector only
when a valid factory is available.

## Acceptance criteria

- A polymorphic test property can switch between two subtypes from the UI.
- The active subtype's scalar and object properties are editable in place.
- Replacing a subtype does not leave a stale pointer in the widget.
- Unknown, null, and read-only cases are safe and visible.
- Enum and ordinary object rendering remain unchanged.
- The project passes the build command in `README.md`.
