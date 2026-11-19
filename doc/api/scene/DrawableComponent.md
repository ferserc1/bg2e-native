# DrawableComponent ownership and export

Header: `<bg2e/scene/DrawableComponent.hpp>`; namespace `bg2e::scene`.
Direct base: [DrawableRenderComponent](DrawableRenderComponent.md).

This is the exclusive authored owner of a `draw::Drawable` cache identity. Use
`Node::drawable()` for owners; generic rendering uses `Node::drawableRender()`.
[TextComponent](TextComponent.md) and [DrawableInstanceComponent](DrawableInstanceComponent.md)
are sibling classes and are not returned as owners.

`drawable()` returns the owner's mutable realization, or a const Drawable through
a const component. `setDrawable()` publishes replacement geometry within the
current identity, updating instances through its geometry revision. It returns
DrawableCacheDiagnostic: check failure and preserve the previous owner.
`rebindResource(cache, handle)` explicitly selects another identity. A second owner
claim, foreign resource or cross-device assignment rejects; clear/release at the
old boundary before rebinding another device. `clearDrawable()` releases the
component's claim, not the cache's entire resident entry.

`resourceHandle()`, `resourcePath()`, `sourcePath()`, `hasPendingExport()` and
`assignmentDiagnostic()` expose identity/source/publication state. Source paths
can exist without a realized model. `clone()` creates independent geometry and a
separate identity/destination, rather than another instance of this owner.
Duplicate legacy owner paths similarly preserve independence and retain pending
export state if physical copying fails. Dependency issues report pending copies,
assignment/load failures and unresolved resources.

`requiresBg2Export() const` is a read-only virtual query, default true. Complete
export checks it before allocating BG2 assets. False skips model export while the
component serializer still runs; custom procedural subclasses need their own
representation. Whole-component `ignoreSerialization` instead omits the component.
Text has its own serializer and exclusion checks; it does not inherit this query.

Normal scene saves coordinate one model per used resource identity, using owner
properties when present or cache originals when ownerless. Instances serialize
independent appearance and cannot independently export the shared model.
See [complete IO](../db/DrawResourceIO.md) and
[ownership architecture](../../architecture/drawable_caches_and_instances.md).
