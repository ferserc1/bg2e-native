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
