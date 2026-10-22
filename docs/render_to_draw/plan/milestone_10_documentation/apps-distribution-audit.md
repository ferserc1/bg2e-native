# M10-04 — Applications, CLI and distribution audit

[Progress](../progress.md) · [Tutorial audit](tutorials-audit.md) · [Next: documentation validation](step_05_documentation-validation.md)

## Baseline and delivered scope

Delivered 2026-10-22 at HEAD `e2a32164a781dbf420a4464b25ddce9feb862c6a`,
with all inherited uncommitted M10-01–03 changes retained. Lead continuation accepts
M10-03; backend/scenario details remain unspecified. Its primitive shared-UBO
source follow-up remains open and is not repaired or validated here.

Audited current application/workflow guides, four executable parsers, current
HTTP/import/Performance ownership, output publication and platform build/package/
standalone source rules. Corrected ten documents: nine maintained doc/ pages and
scripts/PACKAGING.md. No engine/app/example/shader/build-system or executable script
change. A newly discovered standalone script preflight defect is documented as an
unrepaired source follow-up; it is not hidden by a successful documentation audit.

## Evidence catalog

| Artifact | Meaning |
|---|---|
| [apps-distribution-contracts.tsv](apps-distribution-contracts.tsv) | 18 contract families across 17 guide pages and 50 source anchors, with actual conclusions/limits. |
| [cli-option-coverage.tsv](cli-option-coverage.tsv) | 38 parser option-name rows across the three apps and camera example. The bake parser's 25 valued options are all present; standalone help is checked separately. Name presence supplements, not replaces, manual domain/default/precedence inspection. |
| [deployment-shaders.tsv](deployment-shaders.tsv) | 292 collision-free paired stage outputs derived from actual GLSL/Metal relative paths and the CMake compiler rule. Metal outputs require Apple tooling. Includes do not receive independent outputs. |
| [apps-distribution-page-changes.tsv](apps-distribution-page-changes.tsv) | Entry/final hashes of this step's ten document changes, including PACKAGING. Original M10 inventories remain historical snapshots. |
| [apps-distribution-source-hashes.json](apps-distribution-source-hashes.json) | 50 unchanged inspected source/build/script anchors. |
| [apps-distribution-audit.json](apps-distribution-audit.json) | Scope/counts and explicit source findings; no native execution implied. |
| [apps-documentation-audit.json](apps-documentation-audit.json) | Fresh maintained local links/anchors/project includes result. |

## Application and CLI reconciliation

| Product/domain | Current contract and audit result |
|---|---|
| Registered products | apps/model_edit, apps/bg2e_composer and apps/lightmap_generator use targets with matching names. Stored org.bg2e.apps.draw identities and graphics/import scopes remain independent of executable renaming. No preserved second production application is advertised. The 33-product catalog remains current; default app packages exclude examples/tests. |
| Model Edit | Confirmed MainLoop/draw ownership, explicit graphics preferences, docked workspace, focused light-only tools, partial material edits, independent clone resources and complete bool-checked saves. Added direct launch/backend/help guidance. Presets remain distinct from renderer preferences. Integrated bake/preview/adoption uses stopped producers and completion; complete saves materialize temporary map resources. |
| Composer | Confirmed Scene/Model Properties/Chain/table UI, active authored environment selection, root/camera/light restrictions, generation-guarded imports, stopped complete saves and serial integrated scene baking. Replaced obsolete future-batch wording with the actual standalone batch contract. |
| HTTP/import | Corrected Window menu and draw.bg2e_composer.import scope, removed retired asyncLoadGuarded/safeUpdateScene implementation prose. Source has one joinable CPU reader/16 waiting local jobs and one occupied HTTP slot; handlers do not touch graph/GPU. Publication checks distinct request/document/renderer/destination identities and preserves the canonical HTTP reimport parent. Third-party parsing cancellation is cooperative; active realization is synchronous. Replies follow publication/rebinding/loader completion. |
| HTTP protocol | GET /status and POST /import remain bound to 127.0.0.1; default port 8643, accepted port range 1024–49151. JSON/content-type, path/extension, units/axis conversion and 200/400/404/415/500/503 behavior match source. Absolute file paths are recommended; the parser also accepts existing relative paths. A second instance's port conflict is visible. No HTTP requests were sent. |
| Composer Performance | Added actual Window → Performance behavior: live collection needs no capture file; Start/Stop Recording owns JSONL sessions, closing the window only releases the live lease, and normal MainLoop shutdown stops recording. Model Edit/headless UI parity is not invented. Vulkan pipeline-boundary versus Metal encoder-envelope timings remain distinct; no measurement wait or speedup claim. |
| Lightmap CLI | Verified all 25 valued options, exact standalone --help, defaults/ranges/mode/format restrictions and duplicate/error handling. Import binds actual LightmapSettings under bg2e.bake.standalone.v1 with target bake; explicit scalar CLI values apply last regardless of argument order. Invalid arguments/configuration return 2, native/bake/output/cleanup failures 1, successful committed output/cleanup 0 and cooperative SIGINT/SIGTERM cancellation 130/143. No camera frames are recorded. |
| Protected bake output | Strict required-target failure, generated UV2/copy constraints, explicit custom dependencies, linear RGB8 versus native RGB32F and actual model sidecars match source. Authoritative manifest publishes last, retaining old generations; cancellation before commit has no publication. A post-commit cleanup failure can return failure with complete output already present. This is not power-loss durability or interactive adoption atomicity. |
| Camera CLI | Verified fixed/scale dimensions, exact requested frames, graphics-config precedence/unknown policy, opt-out persistence and frozen scene state. Completed linear camera samples precede one final conversion; PNG decode checks RGBA8 byte preservation. Existing final/pending paths reject, and final no-replace hardlink publication requires hardlink support. This example does not use generic DB copy fallback, offer HDR output or export a bake batch. |

Historical M07/M08 evidence still distinguishes lead continuation from specific
native backend, HTTP, numerical, cancellation, installed-package and performance
results. Existing inspector self-tests are not new output/reload evidence.

## Build and distribution reconciliation

| Area | Actual source rule |
|---|---|
| Toolchains | C++20, VULKAN_SDK cache/environment setting; Linux Ninja, macOS Xcode for native dialogs, Windows Visual Studio 17 2022. Metal shaders require Apple's compiler. Project code signing remains disabled. |
| Engine shaders | Relative draw/bake native paths, separate tree targets and unique AIR directories, shared bg2e_paired_shaders, shader-only app refresh and RuntimeShaders install remain. Output suffixes such as _256 are not runtime table capacity limits. Fresh output is required to establish absence of stale artifacts. |
| Distribution | bg2e_distribution recursively depends on the three registered apps plus engine. Distribution is opt-in; packaging disables examples/tests. Non-Apple installs authoritative assets and paired engine shader trees, plus optional target-specific directories. Apple installs bundles through existing SDK/native resource rules. |
| Linux script | Fresh Ninja build; .tar.gz plus unpacked folder; only Linux offers --allow-dirty/--overwrite. External destination must already exist. Overwrite removes named outputs before building. Install/archive stage privately; top-level external .so files are removed, retaining engine variants and relying on target system runtime/graphics/SDL dependencies. |
| macOS script | Xcode, clean checkout, existing external destination, unused folder/archive and ZIP. No dirty-check bypass or overwrite option. App bundles retain native runtime/shader/assets configuration; no signed/universal-portability claim. |
| Windows script | Visual Studio 2022 x64, clean checkout, external destination, unused output and ZIP. Compiler redistributables/non-system DLL imports and Debug PDBs follow install rules; /MD remains the runtime selection. Dynamically loaded plugins/layers are not all discoverable by binary import scanning. |
| Revision/output safety | Matching initialized submodules and HEAD stability are checked. Clean builds recheck working-tree state; explicit dirty Linux packages still identify HEAD without encoding dirty modifications. macOS/Windows install directly into destination and can leave partial output on failure. No atomic package replacement is promised. |
| Standalone intent | Creators copy headers, assets, current-platform precompiled library, deferred draw template and paired shader/helper resources. Windows selects Release then Debug; macOS accepts root then Release/Debug library outputs. Updaters use a dedicated build, replace headers/library/engine trees/helper and preserve custom shader resources outside draw/bake; copy is not transactional. The preflight defect below currently prevents fresh canonical completion. |

The packaging guide's old cross-platform ZIP/no-overwrite wording and incorrect
Linux dependency description are corrected. Sources, install rules and file names
are evidence of routing only: no configured build, installed files, binary exports,
relocated runtime, package, generated project or native shader was inspected here.

## Standalone source defect — owning M09-05 follow-up

All four scripts contain a representative-file test for
bake/uv_surface.frag.spv. The shell scripts also test the corresponding flat Metal
filename on macOS. The current compile_paired_shader_tree preserves the source
subdirectory and emits bake/uv_surface/surface.frag.spv or .metallib instead.
The derived 292-output catalog confirms that no flat output satisfies the test.

Consequently a fresh canonical build cannot pass current creator/updater preflight.
Creators stop before creating a project; updaters build the library first and stop
before copying refreshed headers/library/shaders/helper. A stale flat file in a
reused build tree can conceal the mismatch but is not a valid compatibility fix.
Rebuilding alone does not repair the script-source test.

The affected scripts are create_standalone_project.sh/.ps1 and
update_libraries.sh/.ps1. Correcting their representative bake path and validating
fresh native create/update/launch is an unmet source integration follow-up owned
by M09-05, separate from localized documentation repair. This documentation-only
audit does not edit build-producing scripts or CMake. Guides now state the blocker
rather than presenting current scripts as guaranteed usable. M10-06 must retain
this finding, alongside the prior primitive shared-UBO overlap risk, without
claiming that continuation or corrected wording repaired either source issue.

## Verification and transition

All 1,275 step-entry non-vendored files outside doc/ and scripts/PACKAGING.md retain
their hashes; no executable script/source/shader/CMake edit. PACKAGING is the only
changed file under scripts/ and is documentation. Source matrix paths, option
coverage, derived output uniqueness, known preflight mismatch and document hashes
are checked statically. Fresh maintained navigation/includes and whitespace checks
pass; the JSON report records actual counts and the same two illustrative-header
exceptions. The existing check can be repeated from the repository root:

```sh
python3 docs/render_to_draw/plan/milestone_09_cleanup/audit_documentation.py --output-dir /tmp/bg2e-m10-apps-audit
```

No CMake configure/build, parser executable, shell/PowerShell product script,
shader compilation, GPU/HTTP request, package/creator/updater/install/relocation,
output reload or performance capture was executed. Native evidence remains
lead-owned and unspecified beyond historical reports already recorded.

Lead review: verify corrected app/CLI/deployment guidance and the two explicit
source follow-ups. Optional native checks should record actual command/backend,
resource root, installed/relocated output and decoded artifacts. M10-04 awaits
acceptance; continuation selects M10-05 documentation validation. No new executable
is introduced, and final source correctness is not certified by this audit.

## Source correction delivered — 2026-10-22

The user subsequently authorized source repair. Both findings are corrected in
[source-corrections.md](source-corrections.md): GPU 11/13 serialize shared UBO access,
and all four standalone helpers check the canonical paired shader paths. Earlier
finding/handoff paragraphs and stored audit hashes above are historical snapshots,
not current open issues. Native validation and final lead acceptance remain pending.
