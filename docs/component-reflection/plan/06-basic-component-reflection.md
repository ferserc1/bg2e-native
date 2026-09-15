# Step 06 — Add basic scene component registrations and Transform TRS

## Goal

Add reflection metadata for components that only require existing scalar,
vector, matrix, enum, and action support. Also make TransformComponent usable
without editing a raw matrix for normal workflows.

## Files to modify or add

```text
lib/include/bg2e/scene/TransformComponent.hpp
lib/src/bg2e/scene/TransformComponent.cpp
lib/src/bg2e/scene/reflection/TransformComponentReflection.cpp
lib/src/bg2e/scene/reflection/OrbitCameraComponentReflection.cpp
lib/src/bg2e/scene/reflection/PolarTransformControllerReflection.cpp
lib/src/bg2e/scene/reflection/FixedScaleTransformControllerReflection.cpp
lib/src/bg2e/scene/reflection/ChainReflection.cpp
```

Add equivalent reflection files for any component whose implementation is
placed in a different source directory, while keeping registration files in
the scene reflection area.

## TransformComponent

Add instance accessors for:

- Translation: `glm::vec3`.
- Euler rotation: `glm::vec3`, expressed consistently in degrees for the UI.
- Scale: `glm::vec3`.

Setters should:

- Extract the other TRS terms from the current matrix.
- Recompose with the edited term.
- Preserve the documented no-shear assumption.
- Handle zero scale safely when extracting rotation.

Register translation, rotation, and scale as editable properties. Keep the raw
matrix as an advanced property, preferably read-only if the normal editor is
intended to preserve TRS invariants. Retain `setIdentity`.

## Controller components

Register only persistent editor-facing state:

- Orbit camera: enabled, rotation, distance, center, speeds, limits, bounds,
  and initial values; add `reset()` as an action.
- Polar controller: enabled, azimuth/elevation, distance, target, and Euler
  angles.
- Fixed-scale controller: scale.

Do not expose runtime mouse/key state or private transient fields. Use angle
metadata for angular scalar properties and drag/range metadata where bounds
are meaningful.

## ChainComponent

Register a display name with no properties so the inspector can identify the
component without inventing editable state.

## Acceptance criteria

- TRS editing updates the matrix and preserves unrelated TRS terms.
- Controller properties render with useful categories and limits.
- `OrbitCameraComponent::reset()` is invokable from the inspector.
- Runtime-only state is absent from metadata.
- Each new registration is discoverable by `ComponentInspector`.
- The project passes the build command in `README.md`.
