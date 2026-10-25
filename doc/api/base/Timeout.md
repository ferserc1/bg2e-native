# Timeout

**Header:** `<bg2e/base/Timeout.hpp>`  
**Namespace:** `bg2e::base`

`Timeout` schedules callbacks using a steady clock. It is polled by its owner;
constructing a timer does not create a thread or independently run callbacks.

```cpp
uint32_t add(std::function<bool()> callback, uint32_t delayMs,
             bool executeOnExit = false);
void cancel(uint32_t id);
void executeTimers();
void executeExitTimers();
```

`add` returns a timer ID. When `executeTimers()` observes an elapsed delay, it
runs the callback. Returning false removes the timer; true keeps it and resets
its last-fire time. Missed intervals are not replayed. `cancel(id)` removes a
timer; an unknown ID has no effect.

`executeExitTimers()` invokes and removes each timer marked `executeOnExit`,
regardless of elapsed time or callback result. A repeating timer can therefore
run again on exit. A previously removed or cancelled timer cannot.

```cpp
// Inside a running application:
auto& timers = bg2e::app::MainLoop::current()->timeout();
auto id = timers.add([]
{
    // Perform one deferred action on the loop thread.
    return false;
}, 500);
```

`MainLoop` polls its own scheduler and runs exit callbacks during shutdown.
For an independently constructed `Timeout`, call these execution methods
yourself. Avoid changing the timer vector from a running timer callback or
concurrently from another thread; callbacks are invoked during vector traversal.

## See also

- [Base API](index.md)
- [API reference](reference.md)
