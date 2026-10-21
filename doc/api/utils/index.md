# Utility APIs

**Namespace:** `bg2e::utils`

Utilities shared by applications and scene workflows. This page lists the
implemented utilities for draw workflows and graphics-independent CPU data; the native production resource overloads are removed.

| Type/function | Header | Purpose |
|---|---|---|
| [`TextureCache`](TextureCache.md) | `<bg2e/utils/TextureCache.hpp>` | Weak draw texture realization reuse validated against its Engine/device. |
| [`BlueNoise`](BlueNoise.md) | `<bg2e/utils/BlueNoise.hpp>` | CPU rank tiles, deterministic sampling and explicitly shared GPU noise atlas. |
| [`MaterialModifier`](MaterialModifier.md) | `<bg2e/utils/MaterialModifier.hpp>` | Reusable partial material patches for CPU attributes and draw materials/drawables. |
| [`ViewpointAnalyzer`](ViewpointAnalyzer.md) | `<bg2e/utils/ViewpointAnalyzer.hpp>` | Synchronous candidate-view analysis for the draw scene API. |
| [`SceneCameraUtils`](SceneCameraUtils.md) | `<bg2e/utils/SceneCameraUtils.hpp>` | Camera FOV query and projection-preserving orbit viewpoint application. |

See the [manipulation API](../manipulation/index.md) and
[draw manipulation tutorial](../../tutorials/draw/11_manipulation.md) for
application integration.
