# DrawableRenderComponent

Header: `<bg2e/scene/DrawableRenderComponent.hpp>`; namespace `bg2e::scene`.

The common rendering contract is the direct base of DrawableComponent,
DrawableInstanceComponent and TextComponent. It supplies geometry identity/read
access, readiness, draw engine, drawable/submesh transforms, names/groups,
visibility, effective material/native material inspection and tracing eligibility.
Rendering, bounds, selection and overlays consume this contract.

Use `Node::drawableRender()` to find a generic renderable. `Node::drawable()` keeps
its concrete owner meaning and `Node::drawableInstance()` finds an instance.
One renderable per node remains the editor contract. Text owns generated geometry
and is not a model owner; instances cannot mutate shared meshes.

The legacy `drawable()` view returns a const Drawable for owner/text components
and no Drawable for an instance. Internal retained geometry and frame-use hooks
are protected rendering bridges. Applications must not obtain mutable instance
geometry through the cache's internal resolve bridge.

SelectionItem provides `forRenderable()`, `lockRenderable()`, `lockMaterial()`,
`lockMaterialDefinition()` and `materialEdited()` for identity-validated appearance
editing. Legacy `lockMesh()` stays unavailable for instances. Geometry commands
must explicitly require an owner instead of treating every renderable as mutable.

See [owners](DrawableComponent.md), [instances](DrawableInstanceComponent.md),
[text](TextComponent.md) and [ownership architecture](../../architecture/drawable_caches_and_instances.md).
