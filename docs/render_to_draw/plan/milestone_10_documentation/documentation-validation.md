# M10-05 — Maintained-documentation validation

Delivered 2026-10-22 at HEAD `e2a32164a781dbf420a4464b25ddce9feb862c6a`, retaining
inherited M10-01–04 work. Lead continuation accepts M10-04. This delivery awaits
lead validation; M10-06 final review is next.

## Coverage and corrections

All 229 maintained Markdown pages retain an inventory disposition and are now
reachable from [the documentation index](../../../../doc/index.md). Added missing
root, architecture, DB/geometry reference and GPU reference navigation; previously
16 pages were unreachable. [validation-pages.tsv](validation-pages.tsv) records
entry/final hashes, keep/update decisions and inherited source owners, including
unchanged foundation pages. [validation-page-changes.tsv](validation-page-changes.tsv)
identifies the 16 pages changed in this step. Historical inventory and domain audit
artifacts remain unchanged; their hashes describe their own delivery snapshots.

Corrected loop-owned joined worker lifetime, exception/completion/token behavior,
CPU-only loading, UI callback ownership and actual safe-update snapshot semantics.
The safe-update example now copies the current implementation; callbacks queued
while processing wait for another snapshot, and a throwing callback does not
implicitly retry the remaining local batch. GPU idleness is not producer exclusion.

Refreshed 23 of 30 excerpts labeled exact, including current Surface ownership,
common engine initialization, submission tracking and profiling/cadence code.
All 30 now match source tokens ignoring comments and whitespace, with refreshed
source line captions. [validation-excerpts.tsv](validation-excerpts.tsv) and
[validation-source-hashes.json](validation-source-hashes.json) identify the evidence.
Other snippets retain their domain-audit classification as extracts or recipes;
this check does not establish that standalone snippets compile.

Reviewed six Mermaid diagrams against source ordering and ownership. Corrected
history/denoise before lighting, accumulation before normalization/dilation, and
baker release before generation publication. Updated tracing ABI v2 to current v3,
removed obsolete preview/diagnostic names and transitional render/coexistence
claims, and clarified that primitive sample rebuild policy is not a requirement
of acceleration scope. [validation-diagrams.tsv](validation-diagrams.tsv) records
static review; no diagram renderer was run.

## Static evidence and limits

[validation-documentation-audit.json](validation-documentation-audit.json) records
229 pages, 1,913 local links/anchors, 111 project include occurrences and zero
errors. Two reflection includes intentionally name a user-defined MyComponent.
[validation-audit.json](validation-audit.json) records root reachability, balanced
fences, exact excerpts, five JSON blocks and non-document preservation. Two JSON
blocks demonstrate the engine parser's documented trailing-comma extension;
three pass strict JSON parsing. All 1,276 step-entry non-document files retain
hashes, including executable scripts, source, shaders and build files.

Cross-document review retains the previous API/tutorial/application audit contracts:
R01–R14 optimization/profiling evidence limits, current tracing v3/shadows v5,
device-clamped capacity bounded by 16,384, stored alpha default zero with effective
0.02, current settings identity, bool-checked saves and explicit ownership/drain
boundaries. Removed namespaces and paths in deliberate breaking-change or source
blocker explanations are not live API recommendations. Native sample-count helpers
do not imply public draw MSAA support. Prior tutorial assemblies, CLI option and
shader-output matrices remain supporting static evidence, not execution results.

No configure, compile, native app, test, shader, package, creator/updater, output or
performance execution occurred. External URLs were not fetched; C++ was not parsed
by a compiler. No implementation or CMake changes were made. This document does
not replace the M10-06 final review or its lead acceptance.

## Unresolved source follow-ups for M10-06

1. Primitive GPU examples 11/13 share mutable light/camera UBOs without an all-reader
   completion proof before host writes. Documentation describes the limitation;
   sample source remains unchanged and no native failure was reproduced.
2. All four standalone creators/updaters preflight the obsolete flat
   `bake/uv_surface.frag.spv` path (and corresponding Metal path), while compilation
   emits `bake/uv_surface/surface.frag.spv`. Fresh canonical builds fail preflight.
   This unmet M09-05 integration requirement remains a source blocker; rebuilding,
   documentation acceptance and this static audit do not repair it.

## Source correction delivered — 2026-10-22

The user subsequently authorized source repair. Both findings are corrected in
[source-corrections.md](source-corrections.md): GPU 11/13 serialize shared UBO access,
and all four standalone helpers check the canonical paired shader paths. Earlier
finding/handoff paragraphs and stored audit hashes above are historical snapshots,
not current open issues. Native validation and final lead acceptance remain pending.
