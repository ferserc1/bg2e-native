# Utility APIs

**Namespace:** `bg2e::utils`

Utilities shared by applications and scene workflows. This page lists the
implemented utilities relevant to draw manipulation; it does not imply that
production and draw resources are interchangeable.

| Type/function | Header | Purpose |
|---|---|---|
| [`MaterialModifier`](MaterialModifier.md) | `<bg2e/utils/MaterialModifier.hpp>` | Reusable partial material patches for production and draw materials/drawables. |
| [`ViewpointAnalyzer`](ViewpointAnalyzer.md) | `<bg2e/utils/ViewpointAnalyzer.hpp>` | Synchronous candidate-view analysis for the experimental draw scene API. |
| [`SceneCameraUtils`](SceneCameraUtils.md) | `<bg2e/utils/SceneCameraUtils.hpp>` | Camera FOV query and projection-preserving orbit viewpoint application. |

See the [manipulation API](../manipulation/index.md) and
[draw manipulation tutorial](../../tutorials/draw/11_manipulation.md) for
application integration.
