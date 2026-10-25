# Camera

**Header:** `<bg2e/base/Camera.hpp>`  
**Namespace:** `bg2e::base`

`Camera` stores a projection description and its cached 4×4 matrix. It does not
store a camera's world transform or view matrix.

## Projection ownership and updates

| Member | Behavior |
|--------|----------|
| `setProjection(math::Projection*)` | Takes ownership through a new `shared_ptr`; do not pass a stack object or an already-owned raw pointer. |
| `setProjection(shared_ptr<math::Projection>)` | Shares ownership. |
| `projection()` | Mutable/const borrowed projection pointer. |
| `projection<T>()` | Uses `dynamic_cast`; returns null for an incompatible type. |
| `projectionMatrix()` | Returns the cached matrix without recomputing it. |
| `updateProjectionMatrix()` | Applies the current projection to the cache and returns it; without a projection, keeps the existing matrix. |

The matrix starts as identity. Call `updateProjectionMatrix()` after changing
projection parameters and before consuming the cached value.

## Serialization

`serialize()` writes a `projection` object when one exists, otherwise a
`projectionMatrix`. `deserialize(shared_ptr<json::JsonNode>)` accepts
`PerspectiveProjection` and `OpticalProjection`, or a fallback matrix when no
projection object is present. An unsupported projection type throws
`std::invalid_argument`; an invalid input object is ignored.

## See also

- [Base API](index.md)
- [API reference](reference.md)
