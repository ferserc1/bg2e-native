# M10-05 — Audit links, snippets and maintained-document consistency

[Milestone summary](summary.md) · [Plan index](../index.md) · [Documentation policy](../documentation-policy.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see progress. **Prerequisites:** M10-01–M10-04.

## Scope and deliverable

Audit existing maintained `doc/` pages and M01–M09 documentation handoffs against final source, products and distributed coverage. Record findings and localized corrections for the final M10 review. This step does not postpone earlier documentation authoring to M10.

## Audit work

1. Validate maintained navigation/relative links/anchors and symbol/header/target/resource references across all inventory entries, including pages kept unchanged.
2. Search for obsolete render/scene::vk/native callbacks, aliases/target paths, experimental-opt-in claims, forward/MSAA/clone promises and inconsistent diagrams/shader names. Distinguish deliberate historical discussion from working usage instructions.
3. Compare snippets statically with delivered code; builds/runtime demonstrations require explicit lead authorization. Record unexecuted snippets and unspecified backend coverage honestly.
4. Apply localized consistency corrections and resolve all audit findings; this is an audit of previously authored pages, not deferred creation of the documentation set.

## Verification and exit

All maintained inventory entries have resolved audit dispositions; no broken navigation or obsolete working-code guidance remains.

Perform relevant static checks. Compilation/runtime validation belongs to the project lead unless explicitly requested. Deliver one step, update progress and wait for lead acceptance under the execution protocol. No new engine feature, build-system change or future API documentation is authorized by this audit.

## M03-10 reviewed implementation requirement

Check that visible-window/app initialization snippets actually connect/bind/draw settings, with rebind/cleanup and current defaults. Link actual lead evidence separately from source/static checks; continuation acceptance is not hardware-specific numeric/performance proof. Confirm all M03 documentation obligations were authored during M03-11, not deferred to this audit.

Source and validation boundaries: [M03 technical review](../milestone_03_ray_tracing/review.md). Include these conditions in this step's implementation and exit checks.

## Current engine preservation requirements — 2026-10-21

Check code/snippets/links for current defaults, v3/v5 bindings, revision and
completion APIs, optimization diagnostic switches, Performance/recording controls
and bool writer return handling. No old cutoff 0.5 default, unconditional TLAS
rebuild, all-queued-update drain or detached worker recipe may remain as current
usage merely because it appeared in an earlier accepted milestone report.

Read the [source reconciliation](../code-reconciliation-2026-10-21.md); current engine code overrides historical implementation assumptions. No native/performance result is implied by this source baseline.

## M08-06 consistency searches

Search maintained guidance for stale standalone-not-implemented text, tentative
SceneAssembler/Batch names, root umbrella export assumptions, flat-file output,
byte-clamped float claims, skipped-required-target success, interchangeable camera/
UV samples and old production callback examples. Keep explicitly historical
production descriptions distinct from final usage. Verify read-only artifact
inspection is not described as full geometry reload or native/performance proof.

## Final M09-07 source baseline and handoff

Use [the final review](../milestone_09_cleanup/review.md) and its
[page inventory](../milestone_09_cleanup/documentation-handoff.tsv). M09-06 and
its scene/Metal/VMA corrections are accepted by continuation. Production is
`bg2e::draw`; GPU is its low-level backend abstraction. Retired render APIs and
old application targets have no compatibility promise. The source catalog lists
33 products; native scenario/backend/performance coverage remains unspecified.
M09-08 authors the complete cumulative maintained documentation before M10.
M10 audits that delivered documentation and makes localized corrections; do not
defer missing API, tutorial, build or application authoring to this milestone.

Check all delivered links, names, snippets, diagrams and shader paths against current source and the 317-header evidence catalog. Separate syntax/source checks from executed examples. Preserve exact capacity 16384, bindings and interface versions where documented; never infer performance measurements from retained profiling probes.
