# Step 07 — Reflect Camera and Projection subtypes

## Goal

Expose CameraComponent projection settings through the polymorphic reflection
feature instead of keeping the projection editor entirely ad hoc.

## Files to modify or add

```text
lib/include/bg2e/math/projections.hpp
lib/include/bg2e/base/Camera.hpp
lib/include/bg2e/scene/CameraComponent.hpp
lib/src/bg2e/scene/reflection/CameraComponentReflection.cpp
lib/src/bg2e/math/reflection/ProjectionReflection.cpp
```

If reflection definitions for math types are kept in another existing module,
place the new file there instead, without changing module layering.

## Reflection surface

Register the projection hierarchy with stable keys:

- Base Projection: near and far, with valid positive ranges.
- PerspectiveProjection: fov, using angle metadata and a sensible range.
- OpticalProjection: focalLength and frameSize, with positive ranges.

Expose CameraComponent's camera/projection ownership through the new
polymorphic property API. The replacement callback must construct only the
supported projection types and use `Camera::setProjection`.

The viewport remains runtime-controlled by `resizeViewport` and should not be
editable or serialized through this inspector. Cached projection matrices and
view matrices are read-only implementation details.

## Compatibility

- Preserve existing JSON formats and `Camera::serialize` behavior.
- Preserve `CameraSettings` until the new editor is verified in the application.
- Do not expose both obsolete and active controls for the same projection
  without clear categories.
- Keep projection switching safe when no projection is currently installed.

## Acceptance criteria

- Perspective and Optical projection types are selectable in the inspector.
- Near/far and subtype-specific properties edit the live projection object.
- Switching projection type updates the camera through `setProjection`.
- Viewport resizing and camera serialization remain unchanged.
- The project passes the build command in `README.md`.
