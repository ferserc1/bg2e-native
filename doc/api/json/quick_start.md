# bg2e::json Quick Start Guide

This guide covers the common `bg2e::json` tasks: parsing text and files,
reading values, building trees, and serializing them.

---

## Table of Contents

1. [Include the module](#include-the-module)
2. [Parse a string](#parse-a-string)
3. [Loading files portably](#loading-files-portably)
4. [Parse an input stream directly](#parse-an-input-stream-directly)
5. [Read object and list values](#read-object-and-list-values)
6. [Use defaults](#use-defaults)
7. [Build and serialize a tree](#build-and-serialize-a-tree)
8. [Line endings and trailing commas](#line-endings-and-trailing-commas)
9. [Common pitfalls](#common-pitfalls)

---

## Include the module

```cpp
#include <bg2e/json/all.hpp>
```

Individual headers are also available:

```cpp
#include <bg2e/json/JsonParser.hpp>
#include <bg2e/json/JsonNode.hpp>
#include <bg2e/json/JsonToken.hpp>
```

## Parse a string

```cpp
std::string input = R"({"name":"scene","visible":true})";

bg2e::json::JsonParser parser(input);
std::shared_ptr<bg2e::json::JsonNode> root = parser.parse();

if (!root || !root->isObject()) {
    // Parsing failed or the root has an unexpected type.
}
```

The parser object must remain alive while using the reference returned directly
by `parse()`. Copying it into a `std::shared_ptr`, as above, makes ownership
explicit and is the recommended pattern.

## Loading files portably

When file contents are copied into a string using `seekg(0, end)`, `tellg()`,
and `read()`, open the file in binary mode:

```cpp
#include <fstream>
#include <string>

std::ifstream input(path, std::ios::binary);
if (!input) {
    // Handle the open error.
}

input.seekg(0, std::ios::end);
const std::streampos end = input.tellg();
if (end == std::streampos(-1)) {
    // Handle the size-query error.
}

std::string buffer(static_cast<size_t>(end), '\0');
input.seekg(0, std::ios::beg);
input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
buffer.resize(static_cast<size_t>(input.gcount()));

bg2e::json::JsonParser parser(buffer);
std::shared_ptr<bg2e::json::JsonNode> root = parser.parse();
```

### Why binary mode is required for this pattern

This requirement is about constructing the buffer correctly; it is not a JSON
requirement and does not mean the parser only accepts a particular newline.

Windows text-mode streams translate CRLF bytes in the file into one LF
character while reading. A position obtained at the physical end of the file
can therefore describe more bytes than `read()` returns as translated
characters. If a string is resized from that end position before the read, its
tail may remain filled with `\0` bytes. Those bytes become part of the string
passed to `JsonParser` and can be interpreted as unexpected input after the
JSON root.

Binary mode disables newline translation. Consequently:

- `tellg()` measures the same byte sequence that `read()` copies;
- CRLF, LF, and CR bytes reach the parser unchanged;
- the tokenizer treats each newline representation as whitespace;
- behavior is the same on Windows, Linux, and macOS.

Resizing the string to `gcount()` is retained as a defensive measure for short
reads. Do not normalize line endings manually.

An iterator-based binary read is also concise and avoids a separate size query:

```cpp
std::ifstream input(path, std::ios::binary);
std::string buffer {
    std::istreambuf_iterator<char>(input),
    std::istreambuf_iterator<char>()
};
```

## Parse an input stream directly

The stream constructor avoids an intermediate string:

```cpp
std::ifstream input(path, std::ios::binary);
bg2e::json::JsonParser parser(&input);
std::shared_ptr<bg2e::json::JsonNode> root = parser.parse();
```

The stream must outlive the parser and remain open during `parse()`. Binary mode
is recommended for consistent byte-level behavior across platforms. The parser
itself accepts every supported newline representation.

## Read object and list values

```cpp
if (root && root->isObject()) {
    auto& titleNode = root->objectValue("title");
    auto& pointsNode = root->objectValue("points");

    std::string title = titleNode.stringValue("Untitled");
    if (pointsNode.isList()) {
        for (const auto& point : pointsNode.listValue()) {
            if (point && point->isNumber()) {
                float value = point->numberValue(0.0f);
            }
        }
    }
}
```

`objectValue(key)` returns a null sentinel for a missing key. Check the type or
use a default-value overload before consuming data from untrusted input.

## Use defaults

Default-value overloads return the supplied fallback when the node type does
not match:

```cpp
std::string label = node.stringValue("Unnamed");
float scale = node.numberValue(1.0f);
bool enabled = node.boolValue(true);
glm::vec3 position = node.glmVec3Value(glm::vec3(0.0f));
```

The no-argument accessors throw `std::logic_error` on a type mismatch.

## Build and serialize a tree

```cpp
using namespace bg2e::json;

auto root = JSON(JsonObject {
    { "name", JSON("cube") },
    { "visible", JSON(true) },
    { "position", JSON(glm::vec3(1.0f, 2.0f, 3.0f)) },
    { "tags", JSON(JsonList { JSON("demo"), JSON("mesh") }) }
});

std::string compact = root->serialize();
std::string formatted = root->toString();
```

`serialize()` returns compact JSON. `toString()` returns indented text, and
`printNode()` writes that representation to standard output.

## Line endings and trailing commas

Files using any of these byte sequences between tokens are accepted:

```text
{"a":\n1}     LF
{"a":\r\n1}   CRLF
{"a":\r1}     CR
```

Whitespace can appear between tokens. Newlines inside a quoted string remain
string content; the whitespace rule applies between tokens.

The parser intentionally accepts trailing commas, for example
`{"a":1,}` and `[1,2,]`. This behavior is more permissive than strict JSON and
is preserved for compatibility with existing bg2e data.

## Common pitfalls

- Open files in binary mode when creating an exactly sized string buffer.
- Resize the buffer to `gcount()` after `read()` to handle short reads.
- Keep an externally supplied `std::istream` alive until parsing finishes.
- Check the returned pointer: `parse()` reports syntax errors and may return an
  empty root.
- Numbers are stored internally as `float`; integer and double constructors are
  converted to that representation.
- Use type predicates or default-value accessors for data that may be missing or
  have the wrong type.
