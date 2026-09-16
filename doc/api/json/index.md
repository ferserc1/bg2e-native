# JSON API

The `bg2e::json` namespace provides a small JSON tree, parser, tokenizer, and
serialization API. It is used by engine persistence code and can also be used
directly by applications.

The main entry points are:

- [`JsonParser`](JsonParser.md), which parses a string, C string, or input
  stream.
- [`JsonNode`](JsonNode.md), which stores an object, list, string, number,
  boolean, or null value.
- `JSON(...)`, a family of helpers that creates a `std::shared_ptr<JsonNode>`
  from supported C++ values.

> **Newline portability:** the parser accepts LF (`\n`), CRLF (`\r\n`), and CR
> (`\r`) as whitespace. When a file is first copied into a byte-sized string,
> open it with `std::ios::binary`. This keeps `tellg()` and `read()` in the same
> byte domain on Windows. See [Loading files portably](quick_start.md#loading-files-portably).

---

## Table of Contents

1. [Architecture](#architecture)
2. [Supported values](#supported-values)
3. [Parser compatibility](#parser-compatibility)
4. [A complete example](#a-complete-example)
5. [Where to go next](#where-to-go-next)

---

## Architecture

```text
JSON text or stream
        |
        v
JsonTokenizer  ->  JsonParser  ->  shared_ptr<JsonNode>
                                      |-- JsonObject
                                      |-- JsonList
                                      `-- scalar value
```

`JsonTokenizer` recognizes punctuation and scalar tokens. `JsonParser` builds
the tree. Callers normally interact with `JsonParser` and `JsonNode`; the
tokenizer is exposed for low-level use but is not required for ordinary parsing.

The umbrella header includes the complete module:

```cpp
#include <bg2e/json/all.hpp>
```

## Supported values

| JSON value | `JsonNode::Type` | C++ representation |
|------------|------------------|--------------------|
| Object | `Object` | `JsonObject`, a map of node pointers |
| Array | `List` | `JsonList`, a vector of node pointers |
| String | `String` | `std::string` |
| Number | `Number` | Stored internally as `float` |
| Boolean | `Bool` | `bool` |
| Null | `Null` | No payload |

Convenience conversions also represent `base::Color`, GLM vectors and matrices,
and fixed-size float arrays as JSON lists.

## Parser compatibility

The parser deliberately accepts some input beyond strict JSON. In particular,
existing support for trailing commas in objects and arrays is part of this API:

```json
{
  "enabled": true,
}
```

```json
[
  1,
  2,
]
```

LF, CRLF, and CR line endings are accepted equally. The parser does not require
files to use the native newline convention of the current operating system.

## A complete example

```cpp
#include <bg2e/json/all.hpp>

using namespace bg2e::json;

std::string source = R"({
  "name": "sample",
  "enabled": true,
  "position": [1, 2, 3],
})";

JsonParser parser(source);
std::shared_ptr<JsonNode> root = parser.parse();

if (root && root->isObject()) {
    const std::string& name = root->objectValue("name").stringValue();
    bool enabled = root->objectValue("enabled").boolValue();
    glm::vec3 position = root->objectValue("position").glmVec3Value();
}
```

## Where to go next

- **[Quick start](quick_start.md)** — task-oriented recipes and file loading.
- **[Examples](examples.md)** — complete parsing, construction, and
  serialization examples.
- **[API reference](reference.md)** — symbol and header catalog.
- **[JsonParser](JsonParser.md)** — constructors, lifetime, and error behavior.
- **[JsonNode](JsonNode.md)** — tree access, conversion, and serialization.

