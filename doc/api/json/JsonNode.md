# JsonNode

**Header:** `<bg2e/json/JsonNode.hpp>`  
**Namespace:** `bg2e::json`

`JsonNode` is the value and tree type used by the JSON module. A node stores one
object, list, string, number, boolean, or null value.

## Container types

```cpp
class JsonNode;

using JsonObject = std::map<std::string, std::shared_ptr<JsonNode>>;
using JsonList = std::vector<std::shared_ptr<JsonNode>>;
```

Objects own their child nodes through shared pointers. Object keys are ordered
according to `std::map`.

## `Type`

```cpp
enum class Type {
    Object,
    List,
    String,
    Number,
    Bool,
    Null
};
```

Use `isObject()`, `isList()`, `isString()`, `isNumber()`, `isBool()`, and
`isNull()` to inspect a node before reading it.

Specialized list predicates are also provided: `isVec2()`, `isVec3()`,
`isVec4()`, `isColor()`, and `isMat4()`.

## Construction and assignment

Constructors and `setValue(...)` overloads accept:

- `JsonObject` and `JsonList`;
- strings and characters;
- integer types, `float`, `double`, and `bool`;
- `base::Color`;
- GLM vectors and matrices;
- fixed-size float arrays with 2, 3, 4, 9, or 16 elements.

Numbers are stored internally as `float`. Engine math and color values are
stored as numeric JSON lists. `setNull()` changes the node to null.

## Object and list access

```cpp
JsonNode& objectValue(const std::string& key);
JsonNode& objectValue(const char* key);
const JsonObject& objectValue() const;
const JsonList& listValue() const;
```

The keyed overloads return the child node when it exists. A missing key returns
a shared null sentinel; use `isNull()` or a default-value accessor when the key
may be absent.

The container accessors require the corresponding node type and throw
`std::logic_error` on a mismatch.

## Scalar accessors

Strict accessors return the stored value and throw `std::logic_error` if the
node has the wrong type:

```cpp
const std::string& stringValue();
float numberValue();
bool boolValue();
```

Default-value overloads return the supplied fallback on a type mismatch:

```cpp
const std::string& stringValue(const std::string& defaultValue);
float numberValue(float defaultValue);
double numberValue(double defaultValue);
bool boolValue(bool defaultValue);
```

`numberValue(defaultValue)` is overloaded for the supported signed and unsigned
integer widths as well as `float` and `double`.

## Vector, color, and matrix accessors

```cpp
glm::vec2 glmVec2Value();
glm::vec3 glmVec3Value();
glm::vec4 glmVec4Value();
glm::mat4 glmMat4Value();

std::array<float, 2> vec2Value();
std::array<float, 3> vec3Value();
std::array<float, 4> vec4Value();
std::array<float, 16> mat4Value();
```

Each family also has an overload accepting a default value. These conversions
validate the required list size and that every element is numeric.

## Serialization

```cpp
void printNode(int indentationLevel = 0);
std::string toString(int indentationLevel = 0);
std::string serialize();
```

- `serialize()` produces compact JSON text.
- `toString()` produces an indented representation.
- `printNode()` writes the indented representation to standard output.

## `JSON(...)` factory functions

The `JSON(...)` overload set mirrors the value constructors and returns a shared
node, which makes nested object/list construction concise:

```cpp
using namespace bg2e::json;

auto node = JSON(JsonObject {
    { "name", JSON("camera") },
    { "position", JSON(glm::vec3(0.0f, 1.0f, 5.0f)) },
    { "active", JSON(true) }
});
```

There is no `JSON(nullptr)` overload. Create a default `JsonNode` or call
`setNull()` when an explicit null node is needed.

