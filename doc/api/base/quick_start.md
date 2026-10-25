# Base API Quick Start

## Load an exported function

```cpp
#include <bg2e/base/DynamicLibrary.hpp>
#include <stdexcept>
#include <string>

auto library = bg2e::base::DynamicLibrary::load(pluginPath);
using GetMessage = const char* (*)();
auto function = reinterpret_cast<GetMessage>(library.getSymbol("getMessage"));
if (!function)
{
    throw std::runtime_error("Null getMessage symbol");
}
const char* value = function();
std::string message = value ? value : "";
```

`pluginPath` is the complete platform-specific library path. Keep `library`
alive until all calls and borrowed library data accesses finish. See
[DynamicLibrary](DynamicLibrary.md) and the [tutorial](../../tutorials/plugins/dynamic_library.md).

## Own CPU pixels

```cpp
#include <bg2e/base/Image.hpp>

bg2e::base::Image image(new unsigned char[4]{255, 0, 0, 255}, 1, 1, 4);
image.setColorType(bg2e::base::Color::TypeSRGB);
```

The image deletes the array automatically. Do not copy this owning image.

## Describe a material

```cpp
#include <bg2e/base/MaterialAttributes.hpp>

bg2e::base::MaterialAttributes material;
material.setAlbedo(bg2e::base::Color::White());
material.setMetalness(0.0f);
material.setRoughness(0.7f);
```

These are CPU settings; a renderer must realize them before drawing.
See the [module index](index.md) for the remaining services and types.
