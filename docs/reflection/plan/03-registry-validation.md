# Step 03 — Registry validation: depth-limit helpers

## Goal

Give tools and debug builds a way to verify the `maxObjectDepth` limit.
Because static-init order between translation units is undefined, depth
cannot be checked at registration time — it must be computed at runtime,
once all definitions are registered.

## Files to modify

```
lib/include/bg2e/reflection/Registry.hpp
lib/src/bg2e/reflection/Registry.cpp
```

## API additions to `TypeRegistry`

`TypeRegistry` is the only `BG2E_API` class in the module; the new methods
follow the existing style (camelCase, `std::string` keys, plain returns).

```cpp
class BG2E_API TypeRegistry {
public:
    // ... existing API unchanged ...

    // Depth of the deepest Object-property chain reachable from typeName.
    // 0 = the type has no object properties (or is not registered).
    // Unregistered objectTypeName keys and reference cycles stop the chain
    // (a visited set guards against cycles, which by-value nesting cannot
    // produce but future pointer-based nesting could).
    uint32_t objectChainDepth(const std::string& typeName) const;

    // True if every registered type respects maxObjectDepth. When
    // 'offenders' is non-null it is filled with the names of the types
    // whose chains exceed the limit.
    bool validateObjectDepth(std::vector<std::string>* offenders = nullptr) const;
};
```

## Implementation sketch (`Registry.cpp`)

`objectChainDepth` is a memoizing DFS over object-property edges with a
visited set for cycle protection:

```cpp
uint32_t TypeRegistry::objectChainDepth(const std::string& typeName) const
{
    // Iterative/recursive DFS over properties where
    // p.type == PropertyType::Object, following p.objectTypeName keys.
    // Track visited type names to cut cycles; skip unregistered keys
    // (they contribute 0 further depth).
    // Return the maximum chain length found (0 when no object properties).
}

bool TypeRegistry::validateObjectDepth(std::vector<std::string>* offenders) const
{
    bool ok = true;
    for (const auto& name : typeNames())
    {
        if (objectChainDepth(name) > maxObjectDepth)
        {
            ok = false;
            if (offenders)
            {
                offenders->push_back(name);
            }
        }
    }
    return ok;
}
```

Implementation notes:

- Keep it simple: a small recursive lambda with a
  `std::unordered_set<std::string>` visited guard is fine; the graphs are
  tiny. Memoization is optional — do not over-engineer.
- Unregistered `objectTypeName` keys are **not** an error for this check
  (reflection is optional; a UI shows a fallback label). They simply
  terminate the chain.

## Consumption contract (documented, not implemented here)

Step 04 documents in `doc/api/reflection/`:

- Consumers must not recurse beyond `reflection::maxObjectDepth` levels
  (root instance = depth 0). At the limit, render object properties as
  plain labels without expanding them.
- Editor/debug tooling may call
  `TypeRegistry::get().validateObjectDepth(&offenders)` at startup and log
  violations.

## Acceptance criteria

- `cmake --build build` succeeds.
- With only the existing definitions (`Transform`, `bg2e::base::Light`,
  and — after step 04 — `LightSource`), `validateObjectDepth()` returns
  `true`: the deepest chain is `LightSource` → `bg2e::base::Light` = 1.
