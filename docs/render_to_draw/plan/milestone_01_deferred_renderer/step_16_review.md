# M01-16 — Review M01 code and revise all remaining milestones

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M01 product checks and actual implementation diff.

## Scope and deliverable

- `docs/render_to_draw/plan/milestone_01_deferred_renderer/review.md`
- `docs/render_to_draw/plan/architecture-decisions.md`
- `docs/render_to_draw/plan/milestone_02_manipulation/summary.md`
- `docs/render_to_draw/plan/milestone_03_ray_tracing/summary.md`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [docs/render_to_draw/plan/architecture-decisions.md](../architecture-decisions.md).

## Implementation work

1. Follow the root review protocol and record actual public class names, runtime delegate/setRenderer behavior, component mode accessors, deep-clone semantics and cache overloads.
2. Compare actual MRT/capability/subresource/query binding tables with M02 readback, M03 tracing design and M04 UV targets; update their step contracts and source paths.
3. Compare settings codec identifiers/catalog generation, FSR bridge placement and shader layout with M05 headless and M06–M08 applications; revise pending steps rather than leave compatibility guesses.
4. Record all reference-scene restrictions and verified/unverified backend scenarios. Update M02–M10 for intentional changes and missing requirements; do not mark unrun product tests passed.

## Verification and exit conditions

- A review report has evidence and a planned-versus-actual/impact table covering every pending milestone.
- Pending steps use real M01 APIs; first-milestone exclusions are still scheduled in the appropriate later products.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

This is the mandatory handoff into M02. Advanced RT choices remain explicitly deferred until M03-01, even if M01 query tables suggest a candidate approach.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.

## M01-12 review inputs

Include [reflected-settings.md](reflected-settings.md) and tests/draw/settings_codec in the review: live accessor targets, instance metadata, stale generations/weak lifecycle callbacks, complete import validation and rollback limits, typed numeric-string protocol, persistence precedence, and custom upscaler/effect registration. Record actual lead CPU/persistence/lifecycle coverage separately from agent static checks.

## M01-13 actual contracts to review

Compare [upscaling.md](upscaling.md) against source and lead validation. Inspect open Upscaler injection/reflection, live scale policy/fixed extent recovery, reconstructed final contract, generic state restoration around the SDK, four-slot limit, acquired-slot resources, safe resets/rebinds and explicit camera-only FSR limits. Preserve these contracts when revising M03 and M05–M08; no engine histories or denoise were added by M01-13.

## M01-14 actual UI boundary

Review [settings-ui.md](settings-ui.md) and the actual GraphicsSettingsWindow/ReflectionWidget::editValue/facade queueRestore. Confirm original target accessors are not invoked by transient editors, generation/availability refreshes and weak completion lifetimes, strict codec file/persistence behavior and that production RenderSettingsWindow remains independent. ResourcePicker is the minimal HDR file UI; image/material/picking widget adaptation is still pending.

## M01-15 actual product baseline

Read [working-example.md](working-example.md) and examples/draw/10_deferred_renderer/README.md. Compare draw_deferred_renderer source/target/resource registration, native-free MainLoop integration, four-light scene, both transparency classes, explicit OBJ maps, clone guard, packed-channel/UV1/cutout probes, same-scene renderer replacement, stale edit rejection and optional FSR/application resampler. FinalLayer reconstruction is now selectable at construction for pre-initialization contract validation. Preserve the lead's virtual Window destructor correction. Distinguish implemented probes/counters from runtime evidence; no agent compilation/runtime was performed. The reference checklist explicitly schedules absent advanced effects/editor/bake/offscreen capabilities.

## Review artifact

The delivered [review.md](review.md) contains the source/acceptance baseline, actual ABI/API/ownership tables, verification limits, every pending milestone impact and links to edited files. Execution/acceptance status remains in progress.md; no M02 work was implemented by this review.

## Revised final handoff

This technical review is followed by [M01-17 — Cumulative documentation](step_17_documentation.md) in the same milestone. On review delivery, progress points to that documentation step, with normal lead acceptance required. The review does not close the milestone or authorize the following milestone by itself. Reconcile its report with the documentation handoff after that final step.
