# M10-03 — Delivered tutorials and customization audit

[Progress](../progress.md) · [API audit](api-reference-audit.md) · [Next: apps/build](step_04_apps-build-distribution.md)

## Baseline and result

Delivered 2026-10-22 at HEAD `e2a32164a781dbf420a4464b25ddce9feb862c6a`,
preserving all inherited uncommitted M10-01/02 work. Lead continuation accepts
M10-02; backend/scenario coverage remains unspecified. This step changes tutorial
Markdown and audit/handoff artifacts only. Engine, examples, shaders, CMake and
product scripts retain their step-entry bytes.

Reviewed all 31 maintained tutorial pages: 13 draw and 13 GPU numbered examples,
three application/standalone workflows and two indexes. Every numbered tutorial
maps to its current registered target/source; no obsolete legacy numbered series
or additional example target is introduced. Fourteen pages receive corrections.
The extensive draw 06 correction repairs an unmet M09 cumulative documentation
requirement exposed by this audit; it does not add a missing engine feature or
postpone that repair to a later step.

## Traceable evidence

| Artifact | Scope |
|---|---|
| [tutorial-coverage.tsv](tutorial-coverage.tsv) | All 31 pages, current target/workflow, source/registration and final hash. |
| [tutorial-snippets.tsv](tutorial-snippets.tsv) | 426 C++/GLSL/Metal blocks from the 26 numbered guides: 350 token matches to example sources, 76 reviewed teaching extracts/recipes/alternatives. |
| [tutorial-page-changes.tsv](tutorial-page-changes.tsv) | Step-entry/final hashes for 14 changed maintained pages, separate from prior M10 snapshots. |
| [tutorial-source-hashes.json](tutorial-source-hashes.json) | 189 example/build/shader/engine source anchors, including reviewed lifecycle and synchronization implementations. |
| [tutorial-audit.json](tutorial-audit.json) | Counts, full-assembly results and static evidence limits. |
| [tutorial-documentation-audit.json](tutorial-documentation-audit.json) | Fresh whole-maintained-set links/anchors/project includes check. |

Token matching removes comments/whitespace while preserving lexical tokens,
including string literals. A match establishes source correspondence, not API
availability, correct surrounding setup or successful compilation. Nonmatches
were inspected as staged definitions, selected includes, explanatory expressions,
shader excerpts with ellipses, source-loop infrastructure or corrected teaching
recipes. Their per-page reasons are recorded; they are not silently counted as
identical source. The standalone guide's two caller-supplied API recipes are
reviewed separately from the numbered block count.

Tutorial 12's 17 consecutive C++ blocks and tutorial 13's 22 concatenate to their
entire current main.cpp files ignoring whitespace, including license/comments.
Tutorials 10/11 remain pedagogic constructions with later graphics/UI replacements
and explicit alternatives; they are not claimed to be verbatim whole-file copies.

## Findings and repairs

| Finding | Delivered correction |
|---|---|
| Draw 06 described manual conversion meshes/UBOs/helper submissions, fixed 1024-to-8 mips and a clear-only background. The source now uses EnvironmentGenerator and SkyboxRenderer. | Reconciled generation, retained immutable snapshot, public configuration, actual default sizes/full mip chains/HDR precision, image binding, skybox lifecycle and shader deployment. All six current code blocks match source tokens. GPU tutorial 10 remains the manual face/mip construction reference. |
| Model Edit workflow still launched draw_model_edit. | Launch model_edit, matching current target/source/bundle. |
| GPU Sobel shader snippet omitted '=' in binding 1. | Correct GLSL binding declaration, matching the paired source. |
| Generic ring recipe created storage wrappers without initializing their uniform buffer; prose attributed retention to a forwarding shared_ptr setter and implied parity alone made writes safe. | Initialize uniform storage, retain both rings, use the acquired surface slot and explain that acquisition, not the ring or host write, supplies retirement synchronization. Correct model/camera matrix application order. |
| Several low-level loop excerpts dereferenced unavailable drawables. | Add explicit non-null acquisition guards in the window-loop recipes before using the frame or current slot. These are documentation clarifications, not changes to sample implementations. |
| Offscreen comparison recipe omitted present association and an old source comment claimed no fence; clear-loop prose assigned waits to endFrame. | Preserve acquire/record/present/end/submit/endFrame ordering; explain frame association, tracked completion, nonwaiting endFrame and acquisition/readback waits. |
| Deferred teaching edit helper ignored the renderer, and manipulation group patch invalidated only after a potentially partial mutation. | Invalidate through the generic Renderer hook before mutation; state that generic callbacks drain and require application membership/job checks where documents can change. No typed no-drain capability is attributed to these callbacks. |
| Primitive GPU shared-UBO excerpts could be read as generally safe concurrent update recipes. | Document the source limitation and explicit reader-lifetime requirement; direct application authors to per-slot buffers/sets or completion of all shared readers. No implicit GPU synchronization is attributed to updateUniformBuffer. |
| Scene tutorials lacked a current capability/customization checkpoint and included a historical --no-rt review requirement. | State that the actual scene examples implement no --no-rt switch; distinguish disabling requests through settings from testing unsupported-device fallback. Link actual renderer/upscaler/native table/metadata contracts and preserve alpha/default/clone/settings identities. |
| Minor drift: UI title, standalone native archive wording and complete-file assembly claim. | Identify the shipped sample title as a label, describe the readable native archive and qualify staged versus literal reconstruction. |

The original M01–M09 source/handoff evidence remains historical. Neither the
corrected tutorial nor a lexical match establishes native execution success.

## Contract checks against current source

| Area | Audited behavior and evidence boundary |
|---|---|
| Draw 01–09 | Current MainLoop/delegates, retained scene/UI cadence, GPU resource rings and depth ownership, completion-based cleanup, optional native query/pipeline behavior and paired shader paths. Draw 06 now uses the actual public environment owners. |
| Deferred/customization | Current default GI/reflection requests, disabled FSR/SMAA injection and disabled/1.0 resolution; manipulation explicitly uses scale 0.7. Compatible public objects require actual formats, lifecycle, metadata instance addresses and native shader interfaces. Unsupported SDK fallback is bilinear, independently of tracing support. |
| Manipulation | Capture suppresses scene forwarding; noncaptured releases reach the controller after tolerance-based picking. Temporary pick IDs differ from persistent selection/queue/TLAS identities. Alpha holes remain geometrically pickable. Partial scalar/texture/exact-group/regex patches preserve unrelated edits; viewpoint analysis is synchronous queued work, not an asynchronous utility. Preview/overlay/gizmo owners release before Device. |
| Lightmaps | Source reconstruction preserves synchronous pre-acquisition sample submission, one live target/context/open frame, source/settings generations and global serial. All-target result capture precedes adoption; cancel/failure/unsupported required effects do not publish a neutral success. UV diagnostics are CPU-rasterized/uploads; integrated adoption is not file export or a headless batch scheduler. |
| Camera export | Source reconstruction preserves strict CLI/config precedence, exact completed scene sample count and seconds chronology, linear HDR accumulation before one final conversion, final RGBA8 PNG/readback checks, no-clobber output and owner cleanup. Camera samples are not UV samples or display-history nominal counts. |
| Standalone | Caller owns live stopped windowless Engine; retain Assembly.scene/root, prepare UV2 before realization, freeze final settings and release each baker before its successor. Batch applies no interim map to source materials; resolve the final manifest and custom dependencies explicitly. Model copies omit placement/descendants; native RGB32F and interactive half-float limits remain distinct. |
| GPU 01–13 | Preserve direct GPU initialization/commands, explicit Vulkan descriptor sets versus Metal index namespaces, native shader pairs, retired-slot ownership and query versus pipeline requirements. Primitive examples are not the deferred renderer's full fallback/optimization contracts. |
| R01–R14 | Explicit cutoff fixture 0.5 does not change authored default zero/effective 0.02. Independent clones remain editable even with derived-cache reuse. Current tracing v3/query shadows v5, specialization templates and device-clamped capacity remain. Generic updates drain; no profiling measurement waits or fabricated timing results are introduced. |

Current referenced assets, example source directories and registrations exist.
Asset inspection uses filesystem paths, including tracked OBJ assets that rg may
ignore by default. Paired shader source existence is checked; resource compilation,
installed contents and runtime availability remain separate lead checks.

## Source limitation discovered by static inspection

GPU tutorial 11's primitive source updates one shared light UBO; GPU tutorial 13
updates one shared camera UBO before acquisition. Their native Buffer update
implementations write host-visible contents without waiting for all prior GPU
readers. Retiring one presentation slot does not prove all other readers of one
shared allocation have completed. This is a source-level overlap risk, not a
reproduced Vulkan/Metal failure or a new result for production RendererDeferred.

The tutorials now state the limitation and safe per-slot/completed-reader contract.
No sample-source repair is attempted in this documentation-only step. Carry the
issue into M10-06 as an explicit source follow-up outside the documentation plan;
do not turn lead continuation into proof of overlap-safe execution or silently
claim that tutorial wording repaired the sample binaries. The lead may validate
or authorize a separate source correction; that is not required to finish this
truthful documentation audit.

## Static verification and transition

All 1,276 step-entry non-vendored implementation/build/script/test files outside
doc/ retain their hashes. No C/C++ or shader edits require new style/license checks.
All matrix/report paths and final page/source hashes are checked. Fresh maintained
audit: 229 pages, 1,891 local links/anchors, 111 project includes, zero errors,
with two existing illustrative-header exceptions. Whitespace checks pass.

Repeat the maintained link/include check from the repository root:

```sh
python3 docs/render_to_draw/plan/milestone_09_cleanup/audit_documentation.py --output-dir /tmp/bg2e-m10-tutorial-audit
```

No snippets were compiled or executed. No CMake configure/build, shader compiler,
application/GPU/native probe, file export/reload, package/standalone creator or
profiling capture was run. No all-backend correctness/performance claim follows
from token comparison or earlier continuation acceptance.

Lead review: inspect the corrected tutorial/source mapping, particularly draw 06,
resource retirement and the explicit primitive shared-UBO limitation. Optional
runtime checks remain lead-owned: Vulkan/Metal/no-RT/SDK-unavailable paths,
settings/rebind, selection/clone/UV reset, exact sample/output counts and native
paired-resource deployment. No new executable is introduced. M10-03 awaits lead
acceptance; continuation selects M10-04 apps/CLI/build/distribution audit.

## Source correction delivered — 2026-10-22

The user subsequently authorized source repair. Both findings are corrected in
[source-corrections.md](source-corrections.md): GPU 11/13 serialize shared UBO access,
and all four standalone helpers check the canonical paired shader paths. Earlier
finding/handoff paragraphs and stored audit hashes above are historical snapshots,
not current open issues. Native validation and final lead acceptance remain pending.
