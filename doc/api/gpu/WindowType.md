# WindowType

**Header:** `<bg2e/gpu/Backend.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
enum class WindowType {
    Vulkan,
    Metal
};
```

Identifies the windowing type associated with a backend. Returned by
`Backend::windowType()` to determine the correct SDL window creation flags.

| Value    | Description                                              |
|----------|----------------------------------------------------------|
| `Vulkan` | The backend requires an SDL window with `SDL_WINDOW_VULKAN`. |
| `Metal`  | The backend requires an SDL window with `SDL_WINDOW_METAL`.  |
