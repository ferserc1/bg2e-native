# JSON API Reference

Class, type, and function reference for the `bg2e::json` namespace.

Every symbol lives under `bg2e::json`. The umbrella header
`<bg2e/json/all.hpp>` includes the complete module.

---

## Types

| Type | Header | Description |
|------|--------|-------------|
| [`JsonObject`](JsonNode.md#container-types) | `json/JsonNode.hpp` | `std::map<std::string, std::shared_ptr<JsonNode>>`. |
| [`JsonList`](JsonNode.md#container-types) | `json/JsonNode.hpp` | `std::vector<std::shared_ptr<JsonNode>>`. |
| [`JsonNode::Type`](JsonNode.md#type) | `json/JsonNode.hpp` | Node kind: object, list, string, number, boolean, or null. |
| `JsonTokenType` | `json/JsonToken.hpp` | Low-level tokenizer token kind. |
| `JsonToken` | `json/JsonToken.hpp` | A token kind and optional string value. |

## Classes

| Class | Header | Description |
|-------|--------|-------------|
| [`JsonNode`](JsonNode.md) | `json/JsonNode.hpp` | JSON tree node, typed accessors, engine math conversions, and serialization. |
| [`JsonParser`](JsonParser.md) | `json/JsonParser.hpp` | Builds a `JsonNode` tree from a string, C string, or input stream. |
| `JsonTokenizer` | `json/JsonToken.hpp` | Tokenizes a stream and supports one-token rollback without stream seeking. |

## `JSON(...)` factory functions

[`JSON(...)`](JsonNode.md#json-factory-functions) returns a
`std::shared_ptr<JsonNode>`. Overloads accept:

- `JsonObject` and `JsonList`, by copy or move;
- C strings, `std::string`, and `char`;
- signed and unsigned 8-, 16-, 32-, and 64-bit integers;
- `float`, `double`, and `bool`;
- `base::Color`;
- `glm::vec2`, `glm::vec3`, `glm::vec4`, `glm::mat3`, and `glm::mat4`;
- `std::array<float, N>` for `N = 2, 3, 4, 9, 16`.

## `JsonTokenizer`

```cpp
explicit JsonTokenizer(std::istream* stream);

char getWithoutWhiteSpace();
JsonToken getToken();
bool hasMoreTokens();
void rollBackToken();
```

`getWithoutWhiteSpace()` skips space, tab, LF, and CR. `getToken()` returns the
next token. `hasMoreTokens()` performs a one-token lookahead, and
`rollBackToken()` replays the most recently read token. Token rollback is
implemented without seeking, so it does not depend on platform-specific text
stream offsets.

`JsonTokenizer` is primarily parser infrastructure. Application code normally
uses `JsonParser`.

## Header catalog

| Header | Contents |
|--------|----------|
| `bg2e/json/JsonNode.hpp` | `JsonNode`, `JsonObject`, `JsonList`, and `JSON(...)` factories |
| `bg2e/json/JsonParser.hpp` | `JsonParser` |
| `bg2e/json/JsonToken.hpp` | `JsonTokenType`, `JsonToken`, and `JsonTokenizer` |
| `bg2e/json/all.hpp` | Umbrella header including all of the above |

