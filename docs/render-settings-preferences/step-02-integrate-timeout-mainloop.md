# Step 02: Integrate Timeout into MainLoop

## Files to Modify

- `lib/include/bg2e/app/MainLoop.hpp`
- `lib/src/bg2e/app/MainLoop.cpp`

## Interface Changes in MainLoop.hpp

Add include:
```cpp
#include <bg2e/base/Timeout.hpp>
```

Add public accessor:
```cpp
class MainLoop {
public:
    // ... existing methods ...
    base::Timeout& timeout() { return _timeout; }
    // ...
protected:
    // ... existing members ...
    base::Timeout _timeout;
};
```

## Implementation Changes in MainLoop.cpp

### 1. Call `executeTimers()` each frame

After `drainMainThreadQueue()` (around line 275), inside the `if (!quit)` block:

```cpp
if (!quit)
{
    executeSafeUpdateScene();

    drainMainThreadQueue();

    _timeout.executeTimers();  // NEW — process timers each frame

    if (resizing)
    {
        // ... existing resize timeout logic ...
    }
    // ... rest of frame ...
}
```

### 2. Call `executeExitTimers()` at loop exit

After the `while (!quit)` loop ends (around line 308), before the `persistentSize` save:

```cpp
    } // end while (!quit)

    _timeout.executeExitTimers();  // NEW — fire executeOnExit timers before cleanup

    if (_windowConfig.persistentSize)
    {
        // ... existing window size save ...
    }

    _engine.device().waitIdle();
    // ... rest of cleanup ...
```

## Execution Order at Exit

```
1. SDL_QUIT event received
2. _onExitFunction() called (shows confirmation dialog)
3. If confirmed: quit = true
4. Main loop exits
5. _timeout.executeExitTimers()  ← fires executeOnExit timers (e.g., preferences persist)
6. Window size saved to preferences
7. GPU wait idle
8. Render loop cleanup
9. UI cleanup
10. Engine cleanup
11. SDL window destroyed
```

This ensures the preferences timer fires BEFORE any GPU/resource cleanup, guaranteeing the settings are written to disk while the application is still in a valid state.
