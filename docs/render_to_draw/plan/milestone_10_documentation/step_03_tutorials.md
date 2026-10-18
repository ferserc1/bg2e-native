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

For manipulation tutorials, compare examples against the [M02 review](../milestone_02_manipulation/review.md): selection capture and UI event routing, exact/group/regex partial patches, clone behavior, optional final overlays, `--no-rt`, and synchronous best-view limitations. Do not imply the draw utility alone handles arbitrary reflected scene edits, alpha-aware picking or async viewpoint analysis.

## Verification and exit

Existing tutorials describe supported delivered products and match their actual source; execution evidence remains honestly attributed.

Perform relevant static checks. Compilation/runtime validation belongs to the project lead unless explicitly requested. Deliver one step, update progress and wait for lead acceptance under the execution protocol. No new engine feature, build-system change or future API documentation is authorized by this audit.


## M04-07 reviewed implementation requirement

Compare the lightmap tutorial with draw_lightmaps's actual synchronous pre-acquisition sample submission, all-target adoption, frozen settings/source snapshots, cancellation and preview rebinding. Do not describe the example as a headless scheduler, async service, image exporter or GPU-rasterized UV diagnostic.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).
