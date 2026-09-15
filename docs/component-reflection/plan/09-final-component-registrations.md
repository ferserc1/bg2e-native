# Step 09 — Finish component registrations and inspector cleanup

## Goal

Complete the registrations that depend on enum support and verify the generic
inspector behavior across all scene components, while keeping specialized
editors for drawable/material workflows.

## Files to modify or add

```text
lib/src/bg2e/base/reflection/LinkJointReflection.cpp
lib/src/bg2e/scene/reflection/ChainJointReflection.cpp
lib/src/bg2e/scene/reflection/DrawableComponentReflection.cpp
lib/src/bg2e/ui/ComponentInspector.cpp
```

Add or rename files only as needed to match the existing source organization.

## Chain joints

Register `base::LinkJoint` as a reflected object with:

- `offset` (`Vec3`).
- `eulerRotation` (`Vec3`) as the canonical editable rotation.
- `transformOrder` (`Enum`) with its two valid options.

Register both InputChainJointComponent and OutputChainJointComponent with the
shared mutable `joint()` object accessors. Do not expose both Euler rotation and
the yaw/pitch/roll aliases in the same form.

## DrawableComponent

Keep the generic registration intentionally minimal: display name only. Do
not add list/array reflection for submeshes or materials in this plan. Existing
`DrawableEditor`, `MaterialEditor`, and `TextureWidgets` remain responsible for
those operations and their GPU synchronization.

## Inspector cleanup

Verify that:

- All reflected components display their metadata display name.
- Categories and action buttons are stable when multiple components are open.
- A missing registration still shows the existing diagnostic.
- Resource changes reach the application callback exactly once.
- Removing a component during inspection remains deferred safely.

The `Add Component` button is not part of this plan unless the existing factory
registry is explicitly connected in a later task.

## Final inventory

Confirm registrations exist for Transform, LightSource, Camera, Drawable,
Environment, OrbitCameraController, PolarTransformController,
FixedScaleTransform, Chain, InputChainJoint, and OutputChainJoint.

## Acceptance criteria

- Every target scene component has either useful reflection metadata or an
  intentional display-only registration.
- Light type and link transform order render as working Combos.
- Camera, environment, transform, and controller workflows are usable from the
  generic inspector.
- Drawable submesh/material workflows remain functional in their specialized
  editors.
- The project passes the build command in `README.md`.
