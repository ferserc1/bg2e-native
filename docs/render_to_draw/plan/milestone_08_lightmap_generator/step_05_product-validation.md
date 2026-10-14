# M08-05 — Standalone CLI parity, multiple targets and deployment

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M08-01–M08-04.

## Scope and deliverable

- `apps/draw/lightmap_generator/`
- `apps/lightmap_generator/`
- `doc/apps/lightmap_generator/index.md`
- `scripts/PACKAGING.md`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [apps/lightmap_generator/](../../../../apps/lightmap_generator), [doc/apps/lightmap_generator/index.md](../../../../doc/apps/lightmap_generator/index.md), [scripts/PACKAGING.md](../../../../scripts/PACKAGING.md).

## Implementation work

1. Compare representative current CLI jobs and output semantics with the new executable, including model/prefab/context, accumulation reset, UV2 and per-target metadata.
2. Verify byte/float formats, edge dilation, batch path preflight/overwrite/atomic output and reload of model copies.
3. Run from packaged/installed resource paths without a window/UI; record supported Vulkan/Metal bake results and unavailable hardware accurately.
4. Recheck existing interactive/offscreen products and legacy applications before proposing deletion.

## Verification and exit conditions

- Output images/model metadata are inspected independently; successfully returning from the CLI is not sole evidence of correct maps.
- All required new applications/products have validation evidence sufficient for the retirement review, with unresolved parity gaps explicit.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M09 cannot remove the behavior reference while a required standalone workflow remains unimplemented or unverified under the chosen acceptance policy.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
