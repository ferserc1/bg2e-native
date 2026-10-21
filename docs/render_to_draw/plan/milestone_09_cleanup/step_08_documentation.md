# M09-08 — Update cumulative implementation documentation

[Milestone summary](summary.md) · [Plan index](../index.md) · [Documentation policy](../documentation-policy.md) · [Execution protocol](../execution-protocol.md) · [Progress](../progress.md)

**Execution status:** see progress. **Prerequisites:** Accepted M09-07 technical review and the implemented product/corrections it records. This is a separate required final step, not implemented by adding this plan file.

## Scope and deliverable

Update maintained `doc/` API/reference, conceptual, tutorial, application and build/distribution pages affected by M01–M09, including indexes and retained pages with affected links/examples. Produce `documentation.md` in this milestone directory only during execution, mapping changed/retained/retired pages to actual source, examples and validation. Historical handoff/review files do not replace user documentation.

## Milestone coverage: Canonical production API, retirement and final migration guidance

Reconcile every maintained page with actual final cleanup: draw is production; remove obsolete render/scene::vk/native callbacks, aliases and old executable paths from usage guidance; retire forward/MSAA/experimental-opt-in promises and explain breaking changes without compatibility shims. Update all API indexes, tutorials, apps/CLIs, build/SDK/platform/package/standalone guidance, retained foundation pages/diagrams and source mapping. Preserve historical planning evidence separately. Finish the complete former M10 authoring scope before the final audit.

## Implementation work

1. Follow the [cumulative documentation policy](../documentation-policy.md). Read actual source, accepted corrections and this milestone review; update the previous documentation baseline instead of writing a parallel future API reference.
2. Inventory affected maintained pages and reconcile actual signatures, namespaces, lifecycle/ownership, reflected settings/schema, capabilities, extension hooks, examples/assets/shaders and deployment. Update or retire contradictory guidance and navigation.
3. Add usable reference and focused tutorials/application instructions for the functionality delivered so far. Reuse actual source/example call sites; name backend/SDK limits and evidence honestly. No snippets or usage guides for unimplemented later milestones.
4. Check links/anchors, symbol/header/target/resource paths, snippets and diagrams statically. Preserve accurate foundation pages and the current coexistence/production status. Compilation/runtime validation stays with the project lead unless explicitly requested.
5. Record coverage/dispositions, source mappings, checks, limitations and remaining documentation obligations in `documentation.md`; append the documentation handoff to `review.md` without rewriting its earlier history. Reconcile affected pending steps and progress.

## Exit and transition

- Maintained documentation describes the actual cumulative implementation through M09, with no future feature presented as available.
- Relevant API reference, tutorials, app/CLI/build guidance and navigation are updated; all applicable former M10 authoring requirements are covered or explicitly assigned to an unimplemented milestone owner.
- Source/links/snippet checks and actual validation ownership are recorded; no invented compilation/backend coverage.
- Deliver this documentation step for lead acceptance before entering M10 final documentation audit. Follow the normal one-step execution protocol. M10 audits the accumulated result and does not defer missing authoring work to itself.

## Current engine preservation requirements — 2026-10-21

Reconcile canonical names without erasing current optimization/profiling guides:
alpha/import policy, revisions/bindings, v3/v5 arrays, completion-based acceleration,
pools, lazy signatures, regional fallbacks, narrow typed updates, CPU/GPU Performance
and bool-noexcept saves. Update current API/app/deployment pages at M09; M10 audits
them rather than authoring missing substantive coverage.

Read the [source reconciliation](../code-reconciliation-2026-10-21.md); current engine code overrides historical implementation assumptions. No native/performance result is implied by this source baseline.

## M08-06 documentation preservation input

Use M08-07's authored standalone API/tutorial/app/package coverage as the baseline.
Update it to actual final component/umbrella/entry-point and executable names after
cleanup; preserve typed bake config and native archive/material/manifest semantics
unless a deliberate data migration was implemented. Remove obsolete production
usage pages without losing their still-valid CPU/file concepts or independent
verification guidance. The M08 review's original production/experimental labels
remain historical evidence; final maintained pages reflect actual M09 promotion,
with no compatibility shims, synthetic-as-native claims or optimization rollbacks.

## M09-03 actual prerequisite and delta

Use [the shared API delivery](shared-api.md) and [per-file changes](shared-api-changes.tsv)
alongside the historical manifest and M09-02 delta. Shared render Engine overloads,
scene native resources/bindings/visitors, DB specialized writer and obsolete widgets
are already retired. 153 dependent production renderer headers/sources are also
removed to close incoming dependencies; 23 render files remain, including native
wrappers and authoritative blue-noise data. Do not restore old interfaces to make
remaining source compile.

DrawableComponent::drawable(), SelectionItem::mesh/lockMesh()/meshPtr(),
SelectionManager::selectedMesh() and GizmoComponent::transformDrawable() now address
draw types only. Factory registration accepts name + defaultCreator. GPUSelectionDialog
accepts caller-provided gpu properties; its standalone OpenGL setup remains a native
confinement seam. Generic UI/preview cleanup is draw-only; retain native Vulkan/Metal
work pending proper GPU bridge relocation, without higher-level GPU dependencies.

Registered UV/integrated test targets now call existing public bake probes through
retirement_harness.hpp; old native main bodies are replaced without CMake changes.
M09-05 audits original fixture-specific numerical/material coverage and removes
the stale test_02 private render include path. M09-06 owns actual execution and
full no-RT/optional/deployment/performance matrices. Shared documentation already
updates 27 maintained pages; preserve current accessors and standalone IO coverage.

## M09-04 actual documentation baseline

[Native retirement](native-retirement.md) supersedes the M09-03 pending-native
notes above: both render trees are gone, data lives at utils/blue_noise_data.h,
and UI's native implementation has moved to gpu/vk and gpu/metal GuiRenderer.cpp.
Document public gpu::GuiRenderer primitive borrowing/explicit shutdown and native
texture generation, StandaloneGuiWindow RAII/context order and drawableSize.
Maintained GPU bridge/API, UserInterface and MainLoop architecture pages already
reflect these contracts; retain those corrections during the full cleanup.
No ImGui/native types enter public headers or higher-level dependency enters GPU.

## M09-05 actual build/template documentation baseline

[Product delivery](products-distribution.md) supersedes earlier pending example/
shader/template notes. Old example groups, GLSL tree and shaders/build.sh/build.bat
are gone. Keep current scripts/PACKAGING.md, examples/README.md and registered
probe references. Audit maintained docs for stale shader-frontend links, removed
example paths, static macOS library claims and old standalone template APIs.
Document paired compilation and authoritative install/bundle trees, shader-only
refresh, config-specific binary/shared shader paths and updater replacement of
engine draw/bake directories while preserving custom app resources. Reused output
directories are not purged by this migration; clean artifacts establish absence.
Do not claim executed configure/package/standalone/native results from static scans.

## M09-06 actual catalog and compatibility evidence

Use [final-product-validation.md](final-product-validation.md), final-products.tsv,
public-api-evidence.tsv and umbrella-closure.tsv after M09-07 reconciles actual
contracts. Document 3 apps, 26 draw/GPU examples and 4 registered probes separately
from manual fixtures. Portable umbrella closure has 255 project headers; the full
public catalog has 317, including explicit/backend headers outside that union.
The actual canonical offscreen names still include DrawOffscreenApplicationConfig/
Delegate; do not invent renamed types or forwarding aliases. Preserve generic
CPU/data IO and authored Drawable component identity while documenting deliberate
application source/ABI/settings/output-layout breaks. Source declaration/export
markers and template projection are not binary/native product validation.

## M09-07 concrete authoring input

Use [review.md](review.md), [documentation-handoff.tsv](documentation-handoff.tsv),
[retirement-reconciliation.tsv](retirement-reconciliation.tsv), final-products.tsv
and public-api-evidence.tsv. Account for every inventoried maintained page, new
API page and redirected/deleted page in documentation.md. Author all final
contracts and product/build/tutorial coverage specified in the review and updated
M10 steps now; M10 is a localized audit. Preserve independent foundation and
profiling documentation. Source-only package/native evidence must stay labeled.
