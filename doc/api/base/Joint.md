# Joint and LinkJoint

**Header:** `<bg2e/base/Joint.hpp>`  
**Namespace:** `bg2e::base`

`Joint` is an abstract transform description. `LinkJoint` implements an offset
and Euler rotation, with selectable matrix composition order.

## Joint interface

| Member | Contract |
|--------|----------|
| `transform()` | Borrowed cached matrix, initially identity. |
| `applyTransform(glm::mat4&) const` | Applies the joint to a destination matrix. |
| `clone() const` | Returns a separately owned joint. |
| `serialize() const`, `deserialize(shared_ptr<json::JsonNode>)` | JSON persistence. |
| `factory(shared_ptr<json::JsonNode>)` | Constructs/deserializes `LinkJoint` when `type` is `LinkJoint`; otherwise returns null. |

## LinkJoint

`offset()` / `setOffset` controls translation. `eulerRotation()` /
`setEulerRotation` controls radians. Individual accessors are `yaw` / `setYaw`
for the X component, `pitch` / `setPitch` for Y, and `roll` / `setRoll` for Z.
All start at zero. Setters immediately rebuild the cached matrix.

`transformOrder()` / `setTransformOrder` uses `LinkTransformOrder`:

| Value | Cached matrix composition |
|-------|---------------------------|
| `TranslateRotate = 1` (default) | `T * Rz * Ry * Rx` |
| `RotateTranslate = 0` | `Rz * Ry * Rx * T` |

`applyTransform(destination)` postmultiplies the destination by the cached
matrix. `clone()` copies this state into a new `LinkJoint`.

Serialization writes `type`, `offset`, `yaw`, `pitch`, `roll`, and
`transformOrder`. Deserialization accepts legacy `order` as a fallback;
missing/invalid order defaults to `TranslateRotate`. Missing Euler components
reset to zero on each deserialization. Invalid input objects are ignored.

## See also

- [Base API](index.md)
- [API reference](reference.md)
