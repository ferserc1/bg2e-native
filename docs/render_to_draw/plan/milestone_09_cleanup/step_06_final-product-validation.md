# M09-06 — Draw production validation and zero legacy dependency checks

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md)

**Status:** planned. **Prerequisites:** M09-02–M09-05.

## Scope and deliverable

- `examples/draw/`
- `apps/draw/`
- `tests/`
- `lib/include/bg2e/all.hpp`
- `lib/include/bg2e.hpp`
- `scripts/PACKAGING.md`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [examples/draw/](../../../../examples/draw), [tests/](../../../../tests), [lib/include/bg2e/all.hpp](../../../../lib/include/bg2e/all.hpp), [lib/include/bg2e.hpp](../../../../lib/include/bg2e.hpp), [scripts/PACKAGING.md](../../../../scripts/PACKAGING.md).

## Implementation work

1. Run the agreed core matrix: default deferred/no-RT, query shadows, advanced RT/histories/denoise, FSR/external upscaler, manipulation/MaterialModifier/clones, integrated maps and accumulated offscreen export.
2. Exercise all three applications and final scene/file/config/pixel output round-trips; validate window/headless failures, dialogs/resources and shutdown.
3. Audit transitive umbrella includes, exported types/symbols, shader paths, build/scripts and generated standalone sources for old namespace/Engine/native callbacks.
4. Record compatibility-breaking changes honestly. Data-format readability is distinct from unsupported old application source/ABI; no forwarding layer is restored.

## Verification and exit conditions

- The surviving products meet their actual previous acceptance gates with final draw-only defaults and packaged resources.
- Static and authorized build/runtime evidence shows render/scene::vk/obsolete application support absent; unsupported test machines remain explicitly unverified.

These are future checks. Run compilation only on explicit user request; record missing runtime/backend evidence as pending, never as a pass.

## Downstream comparison

M10 cannot write accurate final docs from a cleanup that left hidden old code/aliases; this check supplies its authoritative product/API list.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
