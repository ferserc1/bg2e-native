# Engine API

Production scene rendering uses draw over Vulkan or Metal; GPU provides the
low-level backend abstraction. Root `<bg2e/all.hpp>` exports the current modules.

- [Plugins](plugin/index.md): native SDK/ABI contracts, component registration, automatic discovery and programmer-owned unloading.
- [Base](base/index.md): CPU data, platform services, dynamic libraries, timers and profiling.
- [Draw](draw/index.md): lifecycle, renderer, resources, effects and settings.
- [GPU](gpu/index.md): devices, commands, capabilities, native boundaries and GUI bridge.
- [Bake](bake/index.md): integrated/standalone UV jobs, settings and typed output.
- [Application](app/index.md), [scene](scene/index.md), [DB](db/index.md).
- [Manipulation](manipulation/index.md), [utilities](utils/index.md), [UI](ui/index.md).
- [Reflection](reflection/index.md), [geometry](geo/index.md), [JSON](json/index.md).

See [migration](../migration_to_draw.md), [draw tutorials](../tutorials/draw/index.md)
and [build/distribution](../architecture/build_and_distribution.md).
