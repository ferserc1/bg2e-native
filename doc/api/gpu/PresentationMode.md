# PresentationMode

**Header:** `<bg2e/gpu/Instance.hpp>`
**Namespace:** `bg2e::gpu`

```cpp
enum class PresentationMode {
    Undefined,
    Windowed,
    Offscreen
};
```

Tracks whether the instance was created for windowed rendering or offscreen
rendering. Returned by `Instance::presentationMode()` after creation.

| Value       | Description                                        |
|-------------|----------------------------------------------------|
| `Undefined` | Instance has not been created yet.                 |
| `Windowed`  | Instance was created with an SDL window.           |
| `Offscreen` | Instance was created in headless mode (no window). |
