# JsonParser

**Header:** `<bg2e/json/JsonParser.hpp>`  
**Namespace:** `bg2e::json`

`JsonParser` converts JSON text into a tree of
[`JsonNode`](JsonNode.md) instances.

```cpp
class JsonParser {
public:
    JsonParser(std::istream* stream);
    JsonParser(const std::string& buffer);
    JsonParser(const char* buffer);

    std::shared_ptr<JsonNode>& parse();
};
```

## Constructors

### `JsonParser(std::istream* stream)`

Parses from an existing input stream. The stream is not owned; it must remain
alive and open until parsing completes.

```cpp
std::ifstream file(path, std::ios::binary);
bg2e::json::JsonParser parser(&file);
auto root = parser.parse();
```

### `JsonParser(const std::string& buffer)`

Copies the supplied string into an internal string stream. The original string
does not need to outlive the parser.

### `JsonParser(const char* buffer)`

Copies the null-terminated text into an internal string stream. Embedded null
bytes are not supported by this overload.

## `parse()`

```cpp
std::shared_ptr<JsonNode>& parse();
```

Parses exactly one root value and returns a reference to the parser's root
pointer. The root can be an object, list, string, number, boolean, or null.
Copy the result to a `std::shared_ptr<JsonNode>` when it must outlive the parser.

The parser catches `std::logic_error`, writes a warning, and returns its current
root pointer. Callers should check the result before accessing it.

```cpp
bg2e::json::JsonParser parser(source);
std::shared_ptr<bg2e::json::JsonNode> root = parser.parse();
if (!root) {
    // Invalid or incomplete input.
}
```

## Whitespace and newlines

Space, tab, LF, and CR are accepted between tokens. Therefore Unix LF, Windows
CRLF, and classic CR files parse the same way.

When loading a whole file into a pre-sized string, use binary mode so Windows
does not translate CRLF while `read()` is filling a buffer sized from
`tellg()`. The full rationale and recommended code are in
[Loading files portably](quick_start.md#loading-files-portably).

## Compatibility behavior

The parser accepts trailing commas in objects and lists. This is intentional
bg2e compatibility behavior even though trailing commas are outside strict
JSON syntax.

