# M09-06 — Draw production validation and zero legacy dependency checks

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M09-02–M09-05.

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

Repeat [M07 product regressions](../milestone_07_bg2e_composer/product-validation.md)
on the canonical app after cleanup: selection/table clipping and Chain controls,
root/clone/complete save, queued HTTP cancel/reimport/stop-join, integrated multi-target
bake cancellation/previews/save and renderer-first shutdown. The lead confirmed the
selection correction before retirement; native package and broader scenario detail
were not specified and must remain accurately attributed.

1. Run the agreed core matrix: default deferred/no-RT, query shadows, advanced RT/histories/denoise, FSR/external upscaler, manipulation/MaterialModifier/clones, integrated maps and accumulated offscreen export.
2. Exercise all three applications and final scene/file/config/pixel output round-trips; validate window/headless failures, dialogs/resources and shutdown. Include model-editor Workspace parity, complete draw resource save/reload, UV2 regeneration, integrated AO/GI completion/cancel/application, best-view analysis and production model_edit preference/input regressions. M06's current execution coverage is unspecified.
3. Audit transitive umbrella includes, exported types/symbols, shader paths, build/scripts and generated standalone sources for old namespace/Engine/native callbacks.
4. Record compatibility-breaking changes honestly. Data-format readability is distinct from unsupported old application source/ABI; no forwarding layer is restored.

## Verification and exit conditions

- The surviving products meet their actual previous acceptance gates with final draw-only defaults and packaged resources.
- Static and authorized build/runtime evidence shows render/scene::vk/obsolete application support absent; unsupported test machines remain explicitly unverified.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M10 cannot write accurate final docs from a cleanup that left hidden old code/aliases; this check supplies its authoritative product/API list.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.
