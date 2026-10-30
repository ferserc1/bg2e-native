# Foundations

## Authored state and intent

Composer's scene is a tree of `Node` objects. Nodes own components; drawable
components refer to draw resources. UI callbacks run while building an
application frame, whereas scene/GPU publication is scheduled at safe graphics
boundaries. A setter only changes state. It does not know whether the change was
an editor action, animation, loading, or navigation, so the implementation
records explicit editor intent instead of observing setters.

A command says what the user asked to change; its memento captures the exact
prior and resulting authored state needed for replay. A notification such as
`onChanged` runs after work and cannot reliably reconstruct intent or rollback.
See [SDK integration](sdk-integration.md).

## Queue and history are different

The queue orders pending work and schedules it at a safe boundary. History holds
successfully applied undoable commands and a cursor. Admission is not execution:
the command can be rejected by a stale target or failed preparation. Only
effective successful work changes the cursor. Undo/redo replay retained state;
they do not enqueue a new editor command.

An interaction transaction groups preview updates into one eventual entry. An
already-visible preview can be adopted by committing its transaction; a
prepared import/lightmap result instead executes once as a normal command.

## Ownership before abstraction

Removing a node from its parent detaches it from the live tree, but a command's
`shared_ptr` retention keeps the same instance available for restoration. This
preserves identity, component state and external references. `weak_ptr` target
references prevent an entry from extending the lifetime of its destination.
A document generation check is still required: a weak pointer may lock while
referring to a stale target in a replaced document.

GPU buffers, environment publications and renderer histories are derived from
authored CPU/resource state. Undo restores authored state, then adapters rebuild
or invalidate those derived representations. Resource cache contents are not
authored state and are not globally mutated by command history.

## Layering

`bg2e::app` knows only the command contract. The SDK provides optional typed UI
interactions. Composer connects those APIs to its scene, graphics scheduler,
renderer effects and document lifecycle. Unrelated applications can continue
without a history object or Composer dependencies.
