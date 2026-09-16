# JSON Examples

These examples use the umbrella header:

```cpp
#include <bg2e/json/all.hpp>
```

---

## Parse a configuration string

```cpp
using namespace bg2e::json;

JsonParser parser(R"({
  "window": {
    "width": 1280,
    "height": 720,
  },
  "fullscreen": false,
})");

auto root = parser.parse();
if (root && root->isObject()) {
    JsonNode& window = root->objectValue("window");
    int width = window.objectValue("width").numberValue(800);
    int height = window.objectValue("height").numberValue(600);
    bool fullscreen = root->objectValue("fullscreen").boolValue(false);
}
```

The trailing commas are intentional and supported by this parser.

## Load a file with any newline format

```cpp
#include <fstream>
#include <iterator>

using namespace bg2e::json;

std::ifstream file(filePath, std::ios::binary);
if (!file) {
    return;
}

std::string bytes {
    std::istreambuf_iterator<char>(file),
    std::istreambuf_iterator<char>()
};

JsonParser parser(bytes);
auto document = parser.parse();
```

Binary mode preserves the source bytes, and the parser recognizes LF, CRLF,
and CR as whitespace. The same code therefore behaves consistently on Windows,
Linux, and macOS.

## Construct a document

```cpp
using namespace bg2e::json;

JsonList lights {
    JSON(JsonObject {
        { "type", JSON("point") },
        { "intensity", JSON(4.0f) },
        { "color", JSON(bg2e::base::Color(1.0f, 0.8f, 0.6f, 1.0f)) }
    }),
    JSON(JsonObject {
        { "type", JSON("directional") },
        { "intensity", JSON(1.5f) }
    })
};

auto environment = std::make_shared<JsonNode>();
environment->setNull();

auto document = JSON(JsonObject {
    { "name", JSON("Example scene") },
    { "lights", JSON(std::move(lights)) },
    { "environment", environment }
});
```

To create null explicitly without relying on a value overload:

```cpp
auto nullNode = std::make_shared<JsonNode>();
nullNode->setNull();
```

## Read vectors and matrices

```cpp
JsonParser parser(R"({
  "position": [1, 2, 3],
  "transform": [
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    5, 6, 7, 1
  ]
})");

auto root = parser.parse();
glm::vec3 position = root->objectValue("position").glmVec3Value();
glm::mat4 transform = root->objectValue("transform").glmMat4Value();
```

Vector and matrix values are JSON lists. The corresponding accessors validate
the list size and element types.

## Serialize compact and formatted output

```cpp
auto document = bg2e::json::JSON(bg2e::json::JsonObject {
    { "enabled", bg2e::json::JSON(true) },
    { "count", bg2e::json::JSON(3) }
});

std::string compact = document->serialize();
// {"count":3,"enabled":true}

std::string formatted = document->toString();
document->printNode();
```

Objects use `std::map`, so serialized object keys follow map order rather than
insertion order.
