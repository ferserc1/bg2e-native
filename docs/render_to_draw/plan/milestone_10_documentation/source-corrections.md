# Source corrections after M10-06

Delivered 2026-10-22 at HEAD `e2a32164a781dbf420a4464b25ddce9feb862c6a`, retaining
inherited uncommitted M10 work. The lead explicitly requested these source repairs
and removal of the corresponding current documentation warnings. Both corrections
are implemented; lead validation and final acceptance remain pending.

## Example ownership and ordering

GPU 11 waits for device work to complete immediately before rewriting its shared
light UBO. It also skips rendering when surface acquisition returns null. GPU 13
waits before acquiring the next frame and writing its shared camera UBO; it
increments the progressive sample count only after successful acquisition. The
resize path already drains device work before changing its resources and UBO.

These primitive examples deliberately serialize frames for a simple, auditable
all-reader completion boundary. GPU 13's shared accumulation images are also
serialized. The waits trade CPU/GPU overlap for simplicity; no performance gain
is claimed. Applications needing concurrent frames can retain per-slot ownership
as taught by tutorial 07. No production renderer/backend API changed.

## Standalone resources

Both shell and PowerShell creators/updaters now test
`bake/uv_surface/surface.frag.spv`, matching the actual paired GLSL output path.
The two shell macOS checks use `bake/uv_surface/surface.frag.metallib` as well.
Other checks, copy policies and build behavior are unchanged. No shader aliases or
CMake changes are needed.

Current tutorials, ShaderLibraries, build/distribution, lightmap app and PACKAGING
guides describe the corrected behavior. Final review, requirements and progress
no longer list these findings as open. Earlier M10 reports and source/page hashes
remain historical snapshots, explicitly superseded for changed paths by this
correction record; they must not be interpreted as current failures or current
hashes. [source-corrections-delta.tsv](source-corrections-delta.tsv) records the
step-entry/final delta; [source-corrections-checks.json](source-corrections-checks.json)
records checks and scope.

## Verification

- Shell syntax passes `bash -n` for both changed shell scripts.
- Fourteen isolated executions of the actual shell preflight blocks cover both
  scripts, Linux/macOS, canonical paths, missing bake stages, obsolete-flat-only
  stages and missing Metal stages. Only canonical fixtures pass. The harness
  uses temporary empty stage files; it never invokes a creator/updater's build,
  project generation or copy operations.
- Both PowerShell canonical stage declarations map to existing GLSL sources.
  PowerShell parsing/execution was not performed because `pwsh` is unavailable.
- Static source checks confirm UBO writes follow a device drain, both examples
  guard null acquisition, and GPU 13 counts only acquired frames. GPL headers,
  inserted block/control-flow style and change boundaries were reviewed.
- Maintained documentation audit: 229 pages, 1,913 local links/anchors, 111 project
  include occurrences, zero errors and two declared illustrative MyComponent
  include exceptions. See [source-corrections-documentation-audit.json](source-corrections-documentation-audit.json).
- Working-tree whitespace and updated plan links are checked. The two unchanged
  full draw tutorial assemblies still reconstruct their main.cpp sources.

No configure, compile, native application, complete creator/updater, package,
shader or performance execution occurred. The correction establishes source
ordering and path consistency; it does not invent native validation results.

## GPU 13 native failure correction — 2026-10-22

The lead subsequently reported a copyImage format/extent validation abort. The
[display correction](rt-display-correction.md) replaces the RGBA-to-BGRA copy with
a surface-format fullscreen pass, paired shaders, normal RGB order, Metal compute
closure and resize rebinding. Tutorial updated; no engine/backend/CMake changes.
Compilation/native confirmation remains pending; rebuild the example/new shaders.
Earlier correction checks describe their original source snapshot.
