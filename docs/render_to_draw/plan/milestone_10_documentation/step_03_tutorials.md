# M10-03 — Audit delivered-example tutorials and customization guides

[Milestone summary](summary.md) · [Plan index](../index.md) · [Documentation policy](../documentation-policy.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see progress. **Prerequisites:** M10-01/M10-02.

## Scope and deliverable

Audit existing maintained `doc/` pages and M01–M09 documentation handoffs against final source, products and distributed coverage. Record findings and localized corrections for the final M10 review. This step does not postpone earlier documentation authoring to M10.

## Audit work

1. Compare existing tutorials to actual delivered deferred/manipulation/lightmaps/offscreen examples, effects/history code and public renderer/pass/subclass/shader/upscaler customization call sites.
2. Check no-RT/SDK-unavailable fallback, matched shader folders/indices, settings round trips, headless precedence, UV/sample/time/color semantics and clone independence.
3. Confirm referenced targets/resources exist and obsolete forward/render tutorials are retired or clearly historical; retain valid current GPU tutorials.
4. Correct local snippet/link drift. Do not create the tutorial collection here: missing guides remain obligations of the milestone that delivered the functionality.

## Verification and exit

Existing tutorials describe supported delivered products and match their actual source; execution evidence remains honestly attributed.

Perform relevant static checks. Compilation/runtime validation belongs to the project lead unless explicitly requested. Deliver one step, update progress and wait for lead acceptance under the execution protocol. No new engine feature, build-system change or future API documentation is authorized by this audit.
