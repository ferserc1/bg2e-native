# M09-02 — Canonical draw application entry points

[Step](step_02_canonical-entrypoints.md) · [Manifest](retirement-manifest.md) ·
[Change ledger](entrypoint-changes.tsv) · [Progress](../progress.md)

## Delivery and scope

Delivered 2026-10-21 at HEAD `1889bdb12ade85d820eaab4a7365760fa9c34f50` with
inherited uncommitted M09-01 inventory/planning changes preserved. The lead's
continuation accepts M09-01; compilation/checks assumed successful under the
protocol, backend/scenario coverage unspecified. This delivery implements M09-02
only and awaits lead validation. Shared/native engine cleanup remains M09-03/04.

Application now stores one draw::RenderLoopDelegate under renderDelegate() and
setRenderDelegate(). Empty/null shared pointer clears it; input/UI registration
stays independent. The alternate drawDelegate accessor/slot and production setter
are removed. Both editor launchers retrieve the draw delegate through the canonical
accessor without changing app IDs, preferences, HTTP import services or settings.

MainLoop::run(Application*) forwards to the existing explicit EngineConfig overload
with platform defaults: Metal on macOS, Vulkan elsewhere. Explicit backend/format/
validation overrides remain. Validation rejects null application, missing graphics,
UI/input delegates and unsupported Metal before SDL/window allocation. Removed
RenderGraphicsExecution and its factory declaration. The existing draw execution
frame, waitIdle and cleanup implementations are unchanged; runtime renderer switching,
scene invalidation, typed update rules, joined workers and profiling remain active.

OffscreenApplication accepts only DrawOffscreenApplicationDelegate and retains
DrawOffscreenApplicationConfig. Removed production config/delegate, Vulkan target/
command path, OffscreenExecutionMode/header/export and branch-selection API. Its
single Engine/Delegate/config/state now express the retained draw lifecycle directly.
The previous draw init/run/cleanup code is preserved modulo member/method renaming,
removal of mode/other-delegate assignments and an explicit initialized-state run guard.
Configuration validation, exact synchronous scene completion, optional final resolve,
callbacks, no-camera finish, first-exception preservation and partial-init cleanup
remain in the same order. No window/UI is initialized for headless jobs.

The production render::DefaultOffscreenApplicationDelegate header/source and umbrella
export are retired in this step because their deleted app base/config would otherwise
break the automatically globbed engine source build. The draw equivalent is unchanged.
This is a required dependency closure moved forward from M09-04, not a compatibility
shim or deletion of the remaining production rendering implementation.

## Products, registrations and identifiers

Removed the 53 manifest-assigned source/build/old app shader files under
apps/model_edit, apps/bg2e_composer and apps/lightmap_generator. No author assets or
vendored code is removed. apps/CMakeLists.txt now registers only the three apps/draw
products. Their existing executable names, bundle/resource paths, app/preferences/
import-server identities and CLI configuration schemas remain unchanged:

- draw_model_edit and draw_bg2e_composer retain backend overrides and ordinary
  scene/material/bake/import/save/settings/Performance workflows.
- draw_lightmap_generator retains strict complete-batch success, typed settings,
  native RGB32F, cancellation/status handling and authoritative lightmaps.json.
- draw_offscreen_application remains the exact-N camera scene PNG product.

The selected step explicitly requires removal of obsolete target registration;
only apps/CMakeLists.txt and examples/CMakeLists.txt change. Old example registrations
are removed now because their old Application/offscreen contracts no longer exist.
All examples/draw and examples/gpu registrations remain. Old example source files
are retained until M09-05's planned retirement, not advertised as supported targets.
No root/lib CMake, package/install/platform/shader compilation rules are changed.

The registered test_01_key_and_mouse is ported to draw FrameContext clear and draw
UI init to keep this existing target coherent with Application. It retains the
same UI/state polling and app ID, exercising one-argument MainLoop::run. It is a
manual input display, not an automated assertion suite. test_02/test_03 native
bake harnesses and CPU test_04 remain unchanged; their later migration owner remains
M09-05. Standalone project templates and full relocated distribution verification
also remain M09-05/06 obligations; no generated-project success is inferred here.

## Preservation and documentation

No draw shader/pass/renderer, gpu primitive/cache/completion, material default,
DB serialization/publication, bake algorithm, import queue or telemetry code is
changed. The two draw API headers change explanatory comments only. R01–R14,
P01–P11 and unspecified native/performance provenance remain in force.

Eleven maintained pages are reconciled with delivered entrypoints: app MainLoop,
Application_and_input, OffscreenApplication, reference and index; draw EngineConfig,
index, quick_start and RenderLoopDelegate; MainLoop architecture; window/UI tutorial.
Existing offscreen section anchors remain usable. The architecture retains draw
context/leases, UI preparation/composition, frame slots, synchronization, async work,
editor and standalone coverage while retiring the obsolete render adapter guidance.
The offscreen probe guide now points at surviving products. Remaining global
production/status/API/data/native documentation belongs to M09-03/08 and M10;
complete render removal is not documented as already available.

## Static verification and limits

Agent checks, with no compilation or engine execution:

- Compared retained offscreen initialization, full run and cleanup function bodies
  with HEAD after only declared renamings/removals and the initialized guard;
  callback/submission/completion/cleanup order matches.
- Compared draw execution frame/waitIdle/cleanup bodies exactly and both editor
  launcher bodies modulo accessor rename; app identities are unchanged.
- Checked 14 modified C/C++ headers/sources for full project GPL, project include
  existence and lexical delimiter balance; reviewed braces, namespace indentation,
  function separation, method groups and private includes. No third-party rewrite.
- Scanned lib and supported app/example/test source for deleted app delegate/config/
  mode/factory/getter names; no surviving source reference remains. This is not a
  zero-render transitive dependency claim: shared UI/scene/native cleanup is next.
- Checked surviving root app/example/test registration endpoints, 53 app retirements,
  current public config/delegate signatures and package discovery rules by inspection.
- Checked maintained Markdown local paths/fences, change ledger hashes, generator
  syntax and git diff --check. No CMake configure, shader compiler, test binary,
  package/generated-project operation, GPU run or profiling capture occurred.

The 73-row implementation/build ledger records before/after hashes and 57 actual
removals (53 old app files, production window execution, mode header and old offscreen
adapter pair). The M09-01 TSVs remain their original source snapshot. Its generator
now recognizes only explicitly logged removals when refreshing the M08 comparison;
unrecorded missing baseline paths reject before writing. It was not rerun to replace
the accepted historical snapshot during this delivery.

## Project lead checks and handoff

1. Reconfigure/build surviving apps/examples/tests on normal platform setup; old app
   and example targets should disappear, the four named draw products remain.
2. Run the key/mouse target through ordinary MainLoop::run; check input/UI, resize,
   minimize/restore, foreground/background pacing and clean exit. Exercise explicit
   Vulkan and macOS Metal through supported draw examples/editors; unsupported Metal
   on other platforms and missing delegates must fail before window allocation.
3. Exercise both editors: scene/import/save, selection/material edits, environment,
   live renderer/settings switching, pause/loading, Performance and teardown. Compare
   optimized diagnostic baselines where available; this step adds no measurement claim.
4. Run tests/draw/offscreen with default/explicit backends and failure paths; check
   exact camera samples/readback/resolve, unrun destruction, null/invalid/reentrant
   initialization and preservation of the first failure through cleanup.
5. Run camera PNG CLI and standalone no-camera UV batch independently. Preserve
   completed callback counts, CLI overrides, typed output/protected sources,
   cancellation/manifest publication and GPU-owner-before-Engine teardown. Include
   no-RT/optional feature and SDK-unavailable cases applicable to each product.

Deliver awaiting validation. M09-03 shared API cleanup is next. Subsequent owners
must use current source plus this retirement delta rather than assuming the old
manifest's source hashes still describe all existing files.
