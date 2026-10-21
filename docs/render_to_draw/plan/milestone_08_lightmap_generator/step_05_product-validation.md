# M08-05 — Standalone CLI parity, multiple targets and deployment

[Milestone summary](summary.md) · [Plan index](../index.md) · [Architecture decisions](../architecture-decisions.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see [progress](../progress.md). **Prerequisites:** M08-01–M08-04.

## Scope and deliverable

- `apps/draw/lightmap_generator/`
- `apps/lightmap_generator/`
- `doc/apps/lightmap_generator/index.md`
- `scripts/PACKAGING.md`

Paths above include current references and proposed outputs. New paths are targets, not files claimed to exist. Public names follow the architecture contract and may be refined with the remaining plan updated.

Current sources to inspect: [apps/lightmap_generator/](../../../../apps/lightmap_generator), [doc/apps/lightmap_generator/index.md](../../../../doc/apps/lightmap_generator/index.md), [scripts/PACKAGING.md](../../../../scripts/PACKAGING.md).

## Implementation work

Carry [M07's evidence split](../milestone_07_bg2e_composer/review.md): the lead
explicitly checked the selection-crash correction, not every native package,
HTTP or bake-output scenario. Validate real standalone output/root/target/resource
identities, cancellation and protected publication independently, and regression-check
Composer's joined imports and integrated batch shutdown before retirement.

1. Compare representative current CLI jobs and output semantics with the new executable, including model/prefab/context, accumulation reset, UV2 and per-target metadata.
2. Verify byte/float formats, edge dilation, batch path preflight/overwrite/atomic output and reload of model copies.
3. Run from packaged/installed resource paths without a window/UI; record supported Vulkan/Metal bake results and unavailable hardware accurately.
4. Recheck existing interactive/offscreen products and legacy applications before proposing deletion.

## Verification and exit conditions

- Output images/model metadata are inspected independently; successfully returning from the CLI is not sole evidence of correct maps.
- All required new applications/products have validation evidence sufficient for the retirement review, with unresolved parity gaps explicit.

Delegate compilation and applicable runtime checks to the project lead; perform relevant nonexecuting static checks yourself. After delivering this step or its correction, update progress to `awaiting_project_lead` and stop. Their instruction to continue accepts the preceding delivery as compiled/checked without failures; record that basis without inventing test/backend details. Follow the [execution protocol](../execution-protocol.md).

## Downstream comparison

M09 cannot remove the behavior reference while a required standalone workflow remains unimplemented or unverified under the chosen acceptance policy.

Record actual API, ownership and verification in the milestone review. Follow [the review protocol](../review-protocol.md) and edit affected pending steps when actual contracts differ.


## M04-07 reviewed implementation requirement

Record actual independent file reload, pixel type/orientation/HDR and per-target mode/UV2 metadata. M04 continuation acceptance and source-level composition checks are not batch export artifacts or backend-specific numeric evidence. Test packaged and incrementally refreshed bake stage libraries, partial failures/cancellation and source protection with real output paths.

Source and evidence: [M04 technical review](../milestone_04_lightmaps/review.md).

## Current engine preservation requirements — 2026-10-21

Regression coverage delegated to the lead must include enabled and diagnostic
baseline paths from tests/draw/{tracing_tables,shadow_bindings,acceleration_reuse,
resource_pools,temporal_invalidation,transparent_regions,update_synchronization}.
Check cutoff 0/below/equal 0.02 and glTF MASK behavior, stable-frame cache hits,
actual edits causing misses, image/BLAS recreation, overflow, pending/aborted
recordings, retained outputs, source replacement and teardown. Compare output and
work counters; do not assert measured speedups from cache existence. Use existing
profiling probes under docs/profiling_and_stats for disabled/live/recording,
unsupported GPU timing and correct frame/session association, without timing waits.

Read the [source reconciliation](../code-reconciliation-2026-10-21.md); current engine code overrides historical implementation assumptions. No native/performance result is implied by this source baseline.

## M08-03 output validation handoff

Use [batch-output.md](batch-output.md) and tests/bake/batch/README.md as the actual
publication/format/protection contract. Validate final-sample and later-target
cancellation, unchanged previous manifests/generations on failure, native float
values above 65504, complete model sidecars after staging removal, declared custom
inputs/OBJ MTLs, alias protection and cooperating concurrent writers. Inspect
crash leftovers separately from graceful rollback; no power-loss guarantee exists.

## M08-04 delivered CLI validation handoff

Validate [the actual CLI](cli-application.md) and tests/bake/cli/README.md. Build
and execute draw_lightmap_generator separately from the preserved production
lightmap_generator. Check both supported backends, typed bake config identity,
CLI-last overrides, strict/skip unavailable-setting diagnostics, generated versus
existing UV2/copy controls and declared exit codes. The context surface is 1x1 and
records no camera frames; --frames counts completed UV samples. Verify active
installed/package paired shaders after both clean and shader-only builds. Resolve
output exclusively through the final manifest, and test cooperative signal
cancellation/cleanup failure boundaries without inferring power-loss durability.

## M08-05 delivered evidence and limits

[Product validation record](product-validation.md) contains the source-backed
parity table, explicit behavior changes, independent artifact inspection and
P01–P11 native/deployment/regression evidence matrix. The new read-only Python
inspector has ten passing synthetic CPU file tests. That is inspector evidence,
not engine/backend output, native model geometry reload, package or measured
performance evidence. C++/shader builds and native product execution remain
lead-owned under the execution protocol. No production/native implementation or
CMake file is changed by this step. M08-06 must carry the unresolved coverage
explicitly rather than infer named backend/scenario results from continuation.
