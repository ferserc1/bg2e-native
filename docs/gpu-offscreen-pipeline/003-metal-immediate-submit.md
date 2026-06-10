# Step 003 — Metal `gpu::Device::immediateSubmit` override

**Type:** Backend (Metal, macOS only)
**Depends on:** 002 (base `gpu::Device::immediateSubmit` declaration)
**Enables:** 006 (Metal image read-back)

## Goal

Override the backend-agnostic `gpu::Device::immediateSubmit` for Metal. A fresh command buffer
is obtained from the graphics queue, wrapped in `metal::CommandBuffer` so the closure sees the
**abstract** type, then `commit()` + `waitUntilCompleted()` provide the synchronization.

All code guarded by `BG2E_IS_MAC`; a throwing stub in the `#else` branch keeps the translation
unit compiling on Linux/Windows.

## API (`lib/include/bg2e/gpu/metal/Device.hpp`)

```cpp
#include <functional>
// ...
void immediateSubmit(std::function<void(gpu::CommandBuffer* cmd)>&& function) override;
```

(The declaration is unconditional — it overrides the abstract method — but the definition body
is split by `BG2E_IS_MAC` in the `.cpp`, matching the file's existing structure.)

No new members are required: `_graphicsQueue` (a `metal::Queue`) already creates command
buffers.

## Implementation (`lib/src/bg2e/gpu/metal/Device.cpp`)

`#if BG2E_IS_MAC` branch:

```cpp
void Device::immediateSubmit(std::function<void(gpu::CommandBuffer*)>&& function)
{
    auto cmdSP = _graphicsQueue.createCommandBuffer(); // shared_ptr<gpu::CommandBuffer> (metal::CommandBuffer)
    auto* mtlCmd = dynamic_cast<metal::CommandBuffer*>(cmdSP.get());
    if (!mtlCmd) {
        throw std::runtime_error("metal::Device::immediateSubmit: unexpected command buffer type");
    }

    cmdSP->begin();
    function(cmdSP.get());     // closure records abstract commands; blit users down-cast to handle()
    cmdSP->end();

    mtlCmd->handle()->commit();
    mtlCmd->handle()->waitUntilCompleted();
}
```

`#else` branch:

```cpp
void Device::immediateSubmit(std::function<void(gpu::CommandBuffer*)>&&)
{
    throw std::runtime_error("Metal backend is not available on this platform");
}
```

## Notes / pitfalls

- Reusing `_graphicsQueue.createCommandBuffer()` is fine and idiomatic for Metal: MTL command
  buffers are single-use, transient objects (example 05 creates one per frame). No reusable
  pool/fence is needed as in Vulkan.
- `metal::CommandBuffer::begin()/end()` must not `commit()` — committing is done here, once,
  after the closure. Verify `end()` does not auto-commit (it currently only finalizes encoders);
  if `beginRendering` was used in the closure, `end()`/`endRendering()` finalizes the render
  encoder as in a normal frame.
- `metal::CommandBuffer::handle()` (already exists, under `BG2E_IS_MAC`) returns the
  `MTL::CommandBuffer*` used for `commit`/`waitUntilCompleted` and, in the read-back step, for
  creating a blit encoder.
- Include `<bg2e/gpu/metal/CommandBuffer.hpp>` and `<bg2e/gpu/CommandBuffer.hpp>` in
  `Device.cpp` if not already present.

## Validation

- `bg2e` builds: macOS compiles/links the override; Linux/Windows compile the throwing stub.
- Nothing calls it yet; no behaviour change to existing examples.
