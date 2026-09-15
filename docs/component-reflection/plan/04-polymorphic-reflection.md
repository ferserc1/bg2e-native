# Step 04 — Add owned polymorphic-object reflection

## Goal

Represent editable owned polymorphic objects such as
`std::shared_ptr<math::Projection>` without weakening the existing by-value
`object()` contract.

## Files to modify or add

```text
lib/include/bg2e/reflection/Property.hpp
lib/include/bg2e/reflection/Builder.hpp
lib/include/bg2e/reflection/TypeInfo.hpp
lib/include/bg2e/reflection/Registry.hpp
lib/src/bg2e/reflection/Registry.cpp
```

No UI code is changed in this step. The result is metadata that can be consumed
by a later widget step.

## Design

Add a distinct property kind, for example `PropertyType::PolymorphicObject`,
rather than overloading `PropertyType::Object`. The existing object contract
must remain in-place-only for by-value members.

The new metadata needs to describe:

- The currently selected subtype key.
- A const pointer getter for the current object.
- A mutable pointer getter for editing the current object.
- A replacement callback that accepts a registered subtype key and replaces
  the owned object through the owning class.
- Available subtype options, each with a stable registry key and display name.

The replacement path must be explicit and type-erased. It must not expose a
raw `shared_ptr<void>` to consumers or make the generic reflection module
depend on `math::Projection`.

Add a builder API dedicated to polymorphic properties. It should require the
owner to provide the current-object accessors and a subtype factory/replacer;
ordinary scalar/object builder methods must not be silently accepted.

Add registry support for subtype metadata and factories. The registry must
allow registration order between translation units and must report unknown
subtypes cleanly.

## Safety rules

- Null owned objects render as an empty/disabled polymorphic property.
- Unknown current subtype is preserved and reported; it must not be replaced
  automatically.
- Replacement is only permitted when the property has a mutable replacement
  callback.
- The existing object-depth validation remains separate from polymorphic
  subtype selection.

## Acceptance criteria

- A small test registration can describe a base pointer with two subtypes,
  inspect the active subtype, edit it, and replace it.
- Existing `PropertyType::Object` registrations compile unchanged.
- No `render`, `scene`, or `ui` dependency is introduced into reflection.
- The project passes the build command in `README.md`.
