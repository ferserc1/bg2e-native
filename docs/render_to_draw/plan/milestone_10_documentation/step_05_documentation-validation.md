# M10-05 — Links, snippets and complete maintained-doc consistency

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M10-01–M10-04.

## Scope and deliverable

- `doc/`
- `examples/draw/`
- `apps/draw/`
- `lib/include/bg2e/`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [doc/](../../../../doc), [examples/draw/](../../../../examples/draw), [lib/include/bg2e/](../../../../lib/include/bg2e).

## Implementation work

1. Validate all relative links/anchors, code-symbol/header paths, namespace/status statements and API references against final source and actual installed/runtime layout.
2. Search all maintained doc files for obsolete rendering types, aliases, old target paths, opt-in experimental instructions and unsupported capability/clone/MSAA promises.
3. Inspect every retained/rewritten page from the inventory, including reflection/gpu/math/base pages with examples or cross-links. Update diagrams and shader naming conventions.
4. Compile/run documentation demonstrations only on explicit authorization; otherwise cross-check snippets against already validated example code and record unverified standalone snippets honestly.

## Verification and exit conditions

- No broken maintained navigation or working-code reference to deleted APIs remains; intentionally historical text is clearly separated from usage instructions.
- The inventory has a resolved disposition for every doc page and a traceable final API/example mapping.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M10-06 records the final documentation-versus-code comparison rather than marking completeness from a clean link checker alone.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
