# Step 01 — Make enum metadata type-safe

## Goal

Make reflected enum getters and setters usable by a generic consumer. The
metadata and `enumOptions` already exist, but `std::any` currently stores the
concrete enum type, preventing `ReflectionWidget` from handling it uniformly.

## Files to modify

```text
lib/include/bg2e/reflection/Builder.hpp
```

No component registrations or UI code are changed in this step.

## Implementation

Update both `TypeInfoBuilder::property` overloads so enum properties are
adapted at the type-erasure boundary:

- The getter returns `int64_t` for enum values instead of the concrete enum
  type.
- The setter accepts `int64_t` and casts it back to the concrete enum type
  before invoking the original setter.
- Non-enum properties retain their current exact `std::any` behavior.
- The compile-time getter/setter type check remains in place.

Keep `propertyTypeOf<Enum>() == PropertyType::Enum` unchanged. `enumValue`
continues to store `(label, int64_t)` pairs.

The implementation must avoid changing the public setter signature accepted by
the builder. Only the erased lambdas should perform the conversion.

## Acceptance criteria

- `base::Light::type` metadata can be read as `int64_t` and written through the
  metadata without an `any_cast` failure.
- Non-enum scalar, vector, matrix, and object metadata behavior is unchanged.
- Existing reflection registrations compile unchanged.
- The project passes the build command in `README.md`.

## Compile boundary

This step is independently compilable because no new enum registration is
required. It can be verified with the existing `LightReflection.cpp`.
