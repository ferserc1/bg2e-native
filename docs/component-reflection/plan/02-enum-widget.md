# Step 02 — Render enum Combo editors

## Goal

Use the existing `Value::comboBox` primitive to render reflected enum
properties. This fixes the current `Light::type` fallback and prepares the
inspector for camera and joint enum properties.

## Files to modify

```text
lib/src/bg2e/ui/ReflectionWidget.cpp
```

## Implementation

Replace the `PropertyType::Enum` fallback in `drawScalarProperty` with:

1. Read the current value as `int64_t`.
2. Convert `metadata.enumOptions` into the labels and corresponding values
   expected by `Value::comboBox`.
3. Draw the Combo using the reflected property label and current value.
4. Call `prop.setter` with the selected `int64_t` when the value changes.

Define behavior for incomplete metadata:

- Empty `enumOptions`: render a disabled diagnostic instead of crashing.
- Current value absent from the options: preserve the current value and show a
  stable fallback label, or append it as a temporary option if that is what the
  existing Combo API requires.
- Read-only enum properties remain disabled like other scalar properties.

Keep `PropertyEditor::Combo` as metadata, but make enum properties use Combo
by default when options exist. Explicit non-Combo editor metadata should not
silently produce an incompatible numeric editor.

## Acceptance criteria

- `base::Light::type` displays Omni, Spot, Directional, and Disabled.
- Changing the selection invokes `setType` with the correct enum value.
- Read-only enums are visibly disabled.
- Existing scalar and object rendering is unchanged.
- The project passes the build command in `README.md`.
