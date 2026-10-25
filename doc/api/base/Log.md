# Log

**Header:** `<bg2e/base/Log.hpp>`  
**Namespace:** `bg2e::base`

`Log` streams values to standard output or standard error using `operator<<`.
Construct it with `Log::Level::Info`, `Warning`, `Error`, or `Debug`.
Warnings/errors use `std::cerr`; information/debug use `std::cout`.
Debug messages are emitted only when `isDebug()` returns true (based on `NDEBUG`).

```cpp
#include <bg2e/base/Log.hpp>

bg2e_log_info << "Plugin loaded" << bg2e_log_end;
bg2e_log_error << "Plugin load failed" << bg2e_log_end;
```

The macros `bg2e_log_info`, `bg2e_log_warning`, `bg2e_log_error`, and
`bg2e_log_debug` construct a temporary logger at that level.
`bg2e_log_end` supplies a newline. Logging does not add a timestamp, level prefix,
or newline automatically.

## See also

- [Base API](index.md)
- [API reference](reference.md)
