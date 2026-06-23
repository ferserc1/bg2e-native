# Step 01: bg2e::base::Timeout — Timer Manager

## Files to Create

- `lib/include/bg2e/base/Timeout.hpp`
- `lib/src/bg2e/base/Timeout.cpp`

## Files to Modify

- `lib/include/bg2e/base/all.hpp` — add `#include <bg2e/base/Timeout.hpp>`

## Interface

```cpp
// lib/include/bg2e/base/Timeout.hpp
#pragma once

#include <bg2e/common.hpp>
#include <functional>
#include <vector>
#include <cstdint>
#include <chrono>

namespace bg2e {
namespace base {

class BG2E_API Timeout {
public:
    // Adds a timer. callback fires after delayMs. Returns timer ID.
    // If executeOnExit is true, callback fires once more at main loop exit
    // regardless of elapsed time.
    uint32_t add(std::function<bool()> callback, uint32_t delayMs, bool executeOnExit = false);

    void cancel(uint32_t id);

    // Call once per frame. Fires due timers.
    void executeTimers();

    // Call once at main loop exit. Fires all executeOnExit timers.
    void executeExitTimers();

private:
    struct TimerEntry {
        uint32_t id;
        std::function<bool()> callback;
        uint32_t delayMs;
        bool executeOnExit;
        std::chrono::steady_clock::time_point lastFireTime;
    };

    std::vector<TimerEntry> _timers;
    uint32_t _nextId = 1;
};

}
}
```

## Implementation Details

### `add()`

```cpp
uint32_t Timeout::add(std::function<bool()> callback, uint32_t delayMs, bool executeOnExit)
{
    uint32_t id = _nextId++;
    _timers.push_back({
        id,
        std::move(callback),
        delayMs,
        executeOnExit,
        std::chrono::steady_clock::now()
    });
    return id;
}
```

### `cancel()`

```cpp
void Timeout::cancel(uint32_t id)
{
    _timers.erase(
        std::remove_if(_timers.begin(), _timers.end(),
            [id](const TimerEntry& e) { return e.id == id; }),
        _timers.end()
    );
}
```

### `executeTimers()`

Called once per frame. For each timer:
- Compute elapsed time since `lastFireTime`
- If elapsed >= `delayMs`: call `callback()`
  - If callback returns `true`: restart (update `lastFireTime = now`)
  - If callback returns `false`: remove the entry

```cpp
void Timeout::executeTimers()
{
    auto now = std::chrono::steady_clock::now();
    for (size_t i = 0; i < _timers.size(); )
    {
        auto& entry = _timers[i];
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - entry.lastFireTime).count();

        if (static_cast<uint32_t>(elapsed) >= entry.delayMs)
        {
            bool keepAlive = entry.callback();
            if (keepAlive)
            {
                entry.lastFireTime = now;
                ++i;
            }
            else
            {
                _timers.erase(_timers.begin() + i);
            }
        }
        else
        {
            ++i;
        }
    }
}
```

### `executeExitTimers()`

Called once at main loop exit. Fires all active timers that have `executeOnExit == true`, regardless of elapsed time. After firing, removes them (no restart).

```cpp
void Timeout::executeExitTimers()
{
    for (size_t i = 0; i < _timers.size(); )
    {
        if (_timers[i].executeOnExit)
        {
            _timers[i].callback();
            _timers.erase(_timers.begin() + i);
        }
        else
        {
            ++i;
        }
    }
}
```

## Integration Points

- Integrated in Step 02 (MainLoop)
- Used by both apps in Steps 05/06 to register the preferences persistence timer
