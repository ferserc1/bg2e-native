# BackendType

**Header:** `<bg2e/gpu/Common.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
enum class BackendType {
    Vulkan,
    Metal
};
```

Selects which GPU backend to instantiate via `Factory::init()`.

| Value    | Description                                      |
|----------|--------------------------------------------------|
| `Vulkan` | Use the Vulkan graphics backend (all platforms). |
| `Metal`  | Use the Metal graphics backend (macOS only).     |
