# Factory

**Header:** `<bg2e/gpu/Factory.hpp>`

**Namespace:** `bg2e::gpu`

Factory owns the current backend wrapper through shared ownership. It is not a
multi-backend context registry. Native context/resources are created separately.
Standalone gpu examples can call init/ backend; draw's MainLoop adapter calls
acquireBackend and its application should not initialize Factory itself.

Source: [lib/include/bg2e/gpu/Factory.hpp](../../../lib/include/bg2e/gpu/Factory.hpp), lines 28–44. Exact excerpt:

```cpp
class BG2E_API Factory {
public:
    static void init(BackendType type);

    static Backend* backend();

    // Atomically prepare and retain a backend. While a lease is held, init()
    // rejects replacement. No GPU instance or native resources are created.
    static std::shared_ptr<Backend> acquireBackend(BackendType type);

    // Retain an existing factory backend; returns empty for caller-owned backends.
    static std::shared_ptr<Backend> retainBackend(Backend& backend);

private:
    static std::shared_ptr<Backend> _backend;
};
```

## Operations and lifetime

- `init(type)` creates/replaces the wrapper, unless an active external lease
  exists (`use_count > 1`), in which case it throws logic_error.
- `backend()` returns a borrowed pointer, throwing runtime_error before init.
  The caller must not delete it; a raw pointer alone does not prevent replacement.
- `acquireBackend(type)` atomically creates and returns a shared lease. It rejects
  existing retained execution/Engine state, even when the requested type matches.
- `retainBackend(Backend&)` returns a lease only if the argument is the current
  factory wrapper; a caller-owned backend produces an empty lease.

All operations lock the private backendMutex. This synchronizes replacement and
lease acquisition, not use of borrowed raw pointers, Instance creation or GPU
resource mutation. Unsupported backend/platform selection throws runtime_error.
Engine checks that its backend matches EngineConfig and that the shared Instance
is not already initialized. Externally owned backends must outlive Engine.

See [Engine ownership](../draw/Engine.md) and
[MainLoop architecture](../../architecture/MainLoop_render_draw_gpu.md).
