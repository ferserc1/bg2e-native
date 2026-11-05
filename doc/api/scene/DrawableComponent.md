# DrawableComponent export policy

**Header:** `<bg2e/scene/DrawableComponent.hpp>` · **Namespace:** `bg2e::scene`

The base component owns a draw::Drawable and optional model source reference;
see [scene contracts](index.md). It now also exposes:

```cpp
virtual bool requiresBg2Export() const;
```

Default true. This is a **read-only virtual query**, not a mutable permission.
Complete scene exporters inspect it before creating a mesh snapshot, collecting
model textures, allocating a BG2 path or changing resourcePath. False skips model
export only: the component's virtual serializer still runs.

[TextComponent](TextComponent.md) overrides false and embeds editable paragraph/
material data. It must not be wrapped in a plain DrawableComponent to bypass the
policy, baked as a model, or have its generated albedo exported. Custom procedural
subclasses need their own representation/resource contract.

This differs from inherited `Component::ignoreSerialization()` /
`setIgnoreSerialization(bool)`, which excludes the **entire component**.
It also differs from texture ignoreSerialization, which omits only a source asset
while retaining UV/sampling fields. Node's exact-then-subclass component lookup
still lets TextComponent participate in draw/picking/material workflows.
