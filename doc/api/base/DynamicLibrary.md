# DynamicLibrary

**Header:** `<bg2e/base/DynamicLibrary.hpp>`  
**Namespace:** `bg2e::base`

`DynamicLibrary` owns one native dynamic-loader reference. It loads executable
code at runtime and resolves exported symbols without exposing platform headers.

## Public API

```cpp
DynamicLibrary() noexcept;
~DynamicLibrary();
DynamicLibrary(const DynamicLibrary&) = delete;
DynamicLibrary& operator=(const DynamicLibrary&) = delete;
DynamicLibrary(DynamicLibrary&& other) noexcept;
DynamicLibrary& operator=(DynamicLibrary&& other) noexcept;

static DynamicLibrary load(const std::filesystem::path& path);
void unload() noexcept;
void* getSymbol(const std::string& name) const;
bool isLoaded() const noexcept;
```

| Operation | Contract |
|-----------|----------|
| Default construction | Creates an unloaded owner; `isLoaded()` is false. |
| `load(path)` | Converts the path to an absolute path and returns a loaded owner by value. Relative paths are relative to the working directory. |
| `getSymbol(name)` | Returns the raw address of an exported symbol; does not infer its type or signature. |
| `unload()` | Releases this owner's native loader reference and clears its handle. Repeated calls are harmless. |
| Destruction | Calls `unload()`, including after a previous manual unload. |
| Move construction | Transfers ownership and leaves the source unloaded. |
| Move assignment | Unloads the destination's previous library, then transfers ownership. Self-move leaves the object unchanged. |

`isLoaded()` reports whether this object holds a handle. It does not report
whether some other owner has loaded the same native module. Native loaders may
retain modules while other loader references exist.

## Resolve and call a function

```cpp
#include <bg2e/base/DynamicLibrary.hpp>
#include <stdexcept>
#include <string>

// pluginPath names the complete library file for the current platform.
auto library = bg2e::base::DynamicLibrary::load(pluginPath);
using GetMessage = const char* (*)();
auto getMessage = reinterpret_cast<GetMessage>(library.getSymbol("getMessage"));
if (!getMessage)
{
    throw std::runtime_error("The getMessage symbol is null");
}
const char* borrowed = getMessage();
std::string message = borrowed ? borrowed : "";
library.unload();
// message owns its bytes; getMessage and borrowed must no longer be used.
```

The caller must use the exact function signature and calling convention exported
by the library. Symbol lookup cannot check them. Keep the owner loaded for every
call and for every access to library-owned data. Synchronize calls and unloading
externally when multiple threads share an owner; the class provides no locking.

## Errors and native behavior

On macOS/Linux, loading uses `dlopen(..., RTLD_NOW | RTLD_LOCAL)`, lookup uses
`dlsym`, and unloading uses `dlclose`. Native load and lookup failures throw
`std::runtime_error` with the loader diagnostic. Lookup clears and checks
`dlerror`, so a null address alone does not indicate a POSIX lookup failure.

On Windows, the implementation uses `LoadLibraryW`, `GetProcAddress`, and
`FreeLibrary`. Paths retain their native wide-character representation. Native
load and lookup failures throw `std::system_error` with the Windows error code.
Path conversion can also throw filesystem exceptions.

Looking up a symbol on an unloaded owner throws `std::logic_error`. Empty names
or names containing embedded null characters throw `std::invalid_argument` when
the owner is loaded. `unload()` is `noexcept`: it clears the handle and does not
report native release failures. No second release is attempted by the destructor.

See the [dynamic library tutorial](../../tutorials/plugins/dynamic_library.md)
for exported C entry points, CMake targets, deployment, and a windowed example.

## See also

- [Base API](index.md)
- [API reference](reference.md)
