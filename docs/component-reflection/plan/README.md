# Scene Component Reflection: Implementation Plan

This plan extends the reflection-driven node inspector to the editable scene
components under `lib/include/bg2e/scene/`. It starts with reflection and UI
dependencies, then adds component registrations and application integration.

The plan incorporates these decisions:

- Camera projections use a new polymorphic-object reflection feature.
- Environment maps use a Resource editor with a file picker.
- Enum properties get working Combo editors.
- Transform editing exposes translation, Euler rotation, and scale while
  retaining the raw matrix as an advanced view.
- `DrawableComponent` remains minimal; submeshes and materials stay in the
  existing dedicated editors.

## Scope

The target components are:

- `TransformComponent`
- `LightComponent`
- `CameraComponent`
- `DrawableComponent`
- `EnvironmentComponent`
- `OrbitCameraComponent`
- `PolarTransformControllerComponent`
- `FixedScaleTransformControllerComponent`
- `ChainComponent`
- `InputChainJointComponent`
- `OutputChainJointComponent`

`Node` properties remain in `NodeEditor`, and `DrawableEditor`/
`MaterialEditor` remain responsible for list-like mesh and material editing.

## Steps

| Step | Document | Result | Depends on |
|---|---|---|---|
| 1 | [01-enum-reflection.md](01-enum-reflection.md) | Enum values are type-erased safely in reflection metadata | None |
| 2 | [02-enum-widget.md](02-enum-widget.md) | `ReflectionWidget` renders enum Combo editors | 1 |
| 3 | [03-resource-editor.md](03-resource-editor.md) | Generic Resource metadata and file-picker widget | None |
| 4 | [04-polymorphic-reflection.md](04-polymorphic-reflection.md) | Reflection can replace and inspect owned polymorphic objects | None |
| 5 | [05-polymorphic-widget.md](05-polymorphic-widget.md) | `ReflectionWidget` renders polymorphic objects and subtype forms | 4 |
| 6 | [06-basic-component-reflection.md](06-basic-component-reflection.md) | Transform TRS and simple scene components are reflected | 1, 2 |
| 7 | [07-camera-reflection.md](07-camera-reflection.md) | Camera projections and their editable parameters are reflected | 2, 5 |
| 8 | [08-environment-resource-integration.md](08-environment-resource-integration.md) | Environment map selection is integrated with scene texture loading | 3 |
| 9 | [09-final-component-registrations.md](09-final-component-registrations.md) | Remaining component registrations and inspector cleanup are complete | 1, 2, 6, 8 |

## Build boundary

Every step must leave the source tree compilable. The project uses recursive
source globs, so no CMake file should be modified. When a new source file is
added, reconfigure before building:

```sh
cmake -S . -B build -G Ninja -DVULKAN_SDK=$VULKAN_SDK
cmake --build build
```

The plan does not require running the build while writing the plan. The build
command is the acceptance check for each implementation step.

## General constraints

- Do not modify CMake files.
- Keep `bg2e::gpu` untouched; this work belongs to reflection, UI, scene, and
  application integration.
- Do not move material or submesh editing into the generic reflection system.
- Generic reflection setters must not require a `render::Engine`; resource
  loading is coordinated by the editor/application layer after a change.
- Preserve the existing `ComponentInspector` fallback for components without
  reflection metadata.
