# M09-05 — Canonical targets, probes and distribution

[Progress](../progress.md) · [Step](step_05_targets-tests-packages.md) ·
[Product delta](product-changes.tsv) · [Probe dispositions](probe-dispositions.tsv)

## Delivery and provenance

M09-04 is accepted by the project lead's continuation instruction under the
execution convention; compilation/checks are assumed successful with backend and
scenario details unspecified. This step preserves all inherited uncommitted work
at HEAD `1889bdb12ade85d820eaab4a7365760fa9c34f50` and awaits lead validation.

The actual delta contains 173 removed, 14 modified and three added product files:
98 files from the 19 legacy example groups (01..18/debug-app), 73 legacy engine
GLSL stages/includes and the two obsolete flat shader compiler scripts are
removed. No current draw/GPU example or app is deleted. The source/hash ledger
covers CMake, templates, scripts, fixtures and accompanying product documentation.
Original M09-01 inventory snapshots are preserved. The inventory generator reads
all four subsequent delta ledgers and can emit current evidence to a separate
directory. Its legacy matcher now distinguishes retired scene::Drawable from
canonical scene::DrawableComponent; matching the latter was a lexical false positive.

The requested step explicitly includes build-system retirement/repair. CMake
changes are limited to lib/CMakeLists.txt, cmake/utils.cmake,
cmake/standalone_utils.cmake, cmake/package_apps.cmake and the obsolete private
include path in tests/test_02_uv_surface/CMakeLists.txt. Root/app/example/test
registration lists required no further changes after M09-02. No configure,
compilation, shader compiler, executable, C++ test, package creator, standalone
creator or library updater was run. Shell parsing used bash -n only; PowerShell
was inspected as source and was not executed or parser-validated.

## Actual deployment contracts

| Surface | Final source behavior |
|---|---|
| Engine compilation | Only paired draw and bake trees compile. Separate tree targets and the bg2e_paired_shaders aggregate, native main entry points, per-tree Metal AIR paths and RuntimeShaders install paths remain unchanged. Generic custom-app GLSL, ShaderLib and Metal helpers still have legitimate consumers and remain. |
| Shared shader includes | 290 quoted local/tree include edges resolve within retained paired roots. The similarly named old GLSL libraries were not their dependencies. All 322 retained paired source/include files remain byte-identical, including tracing v3, shadow v5, `_256` templates and SMAA stages. |
| Development app bundles | Engine shader copies select draw/bake trees rather than the whole historical output root. The existing shader-only incremental refresh target and aggregate dependency remain; native app-specific stages, assets, Vulkan loader/configuration and Metal frameworks retain their existing paths. Existing flat files in a reused bundle are not actively purged; clean/recreated bundles provide artifact-absence proof. |
| Distribution | Recursive collection still finds the three draw app targets. Non-Apple install copies authoritative assets and paired engine trees, plus optional per-app shader subdirectories. It does not use an arbitrary first app or collect stale flat shader outputs. Apple keeps existing native bundle/deployment rules. Packaging scripts still use fresh isolated build trees and their existing clean-checkout/destination policy. |
| Standalone template | DefaultRenderLoopDelegate owns RendererDeferred; scene owns a loaded draw cube, environment and camera with zero-to-one projection. Separate no-op input and wrapper-only UI delegates use canonical Application setters. Scene resources release with delegate cleanup before engine teardown; no external persistent resource owner remains. |
| Standalone creation | Linux/macOS and Windows copy canonical headers and only draw/bake shader trees. They require representative G-buffer and UV surface SPIR-V files; macOS also checks Metal libraries. macOS accepts root or Xcode Release/Debug library output, using the resolved library consistently. Quoted Vulkan loader glob copying is corrected without changing SDK ownership. |
| Standalone updating | Both updaters retain their dedicated build tree, generators and config-specific binary paths, then validate and refresh headers, platform library, full draw/bake trees and standalone_utils.cmake. Shader input stays PRODUCT_DIR/shaders rather than a binary config subfolder. Engine trees are replaced; custom resources elsewhere are preserved. Update is not transactional. |
| Standalone bundling | The copied helper bundles draw/bake independently, retaining custom app shader paths. Legacy flat outputs in an existing project remain user-owned/unbundled as engine resources; no broad deletion of user shaders is introduced. |

Application source, identities/preferences and Composer HTTP/server behavior are
unchanged. Engine headers/sources, optional FidelityFX integration, native GPU
bridges/caches/pools/completion/timing/profiling, SDK deployment and SMAA vendor
data/licenses are untouched. Historical product binary compatibility is not added.
Presence checks in creator scripts are not proof of ABI compatibility or a full
shader inventory; lead validation must use newly built matching libraries/headers.

## Registered coverage versus retired implementation assertions

See [registered-probes.md](../../../../tests/bake/registered-probes.md) for exact
coverage. Existing target names remain:

| Target | Current coverage and disposition |
|---|---|
| test_01_key_and_mouse | Keep M09-02 canonical input/UI fixture and event behavior unchanged. |
| test_02_uv_surface | Existing CPU/GPU UV probe plus original two-island geometry, two submesh identities, known covered/empty texels, typed clear, UV1/UV2 preview and boundary coverage. Remove stale private render include path. |
| test_03_integrated_baker_context | Existing output/composition/integrated lifecycle probes plus original baked RGB G-buffer fixture and receiver/overhang numerical AO/GI expectations through public GPU/draw/bake APIs. Uses a known colored linear sky. |
| test_04_uv2_atlas | Keep target and CPU atlas assertion source byte-identical. |
| Manual fixture families | All pre-existing fixtures/inspectors preserved, including M01 probes, selection/editor/schema/clone/save/import, standalone/assembly/batch/CLI/product, tracing/capacity/cache/completion/pool/region and profiling checks. 119 current test files have explicit keep/adapt dispositions; these are not newly registered or executed suites. |

The new material fixture checks ordinary AO versus explicit baked flags, six
camera color outputs plus D32 depth, independent RGB values 230/82/31 in the
baked half-float attachment and scalar AO remaining one. The numerical fixture
retains original geometry, isolated AO >0.995, overhang contrast >0.08, neutral
padding, exact reset count, RGB8 packing and finite colored RGB32F GI with >0.02
channel separation. Existing probes retain exact-N/two-sample completion,
chart-aware dilation/radius/means, exposure, HDR, metadata and pending cancellation.

Intentional current-contract differences: explicit IndirectTextureMode replaces
implicit AO-texture-means-baked; UV pass exposes five typed attachments with
UINT_MAX clear and v=0 at top, rather than old four-attachment manager/zero clear;
final composition alpha is one, with validity carried separately. Retired bake
rtShadows toggles/direct-shadow multiplication are not reintroduced into the
indirect-only bake settings. Direct query-shadow probes remain. Old native fence,
descriptor and recycled-command-handle assertions are replaced by public serial,
completion and retained-owner checks, with native completion fixtures preserved.
No old renderer wrapper, native command or compatibility settings are recreated.
These are ports to the accepted contracts, not claims that all native numerical
thresholds or every historical implementation detail have passed validation.

## Agent verification

| Static check | Result |
|---|---|
| audit_product_sources.py | 190 delta hashes checked; 1,205 runtime/build/script sources without retired APIs/paths; 33 literal app/example/test subdirectory registrations resolve; 292 paired native stage outputs have no path collision; 290 shader include edges resolve. |
| audit_native_sources.py | 707 supported engine/app/example/test C/C++ files pass include/retired/native-layer/public-header checks; M09-04 hashes and exact noise relocation still valid. |
| Updated inventory into temporary directory | 1,211 source/config files, 3,132 include edges, zero legacy matches and no unrecorded missing M08 baseline path. Broad keep/migrate classification is historical planning metadata, not pending API work. |
| Preservation snapshot | All 588 pre-existing engine headers/sources, 322 paired shader files, 52 app files and 176 supported draw/GPU example files byte-identical. Registered CPU atlas/input and all pre-existing manual C++ fixture bodies unchanged; only the shared registered bake harness is extended. |
| Source/script/docs | Five new/modified C++ files reviewed for GPL prefix, source delimiters, include resolution, braces and ownership/signatures; bash -n for shell creators/updaters/package scripts; changed-page links and git diff --check. No test/binary execution. |

Reproduce source-only checks with:

```sh
python3 docs/render_to_draw/plan/milestone_09_cleanup/audit_product_sources.py
python3 docs/render_to_draw/plan/milestone_09_cleanup/audit_native_sources.py
python3 docs/render_to_draw/plan/milestone_09_cleanup/inventory_sources.py --output-dir /tmp/bg2e-m09-05-inventory
bash -n scripts/create_standalone_project.sh scripts/update_libraries.sh scripts/package_linux.sh scripts/package_macos.sh
```

Lexical scans do not preprocess C++/CMake or validate native shader entry points.
Counts are static evidence, not installed/native/backend/performance results.

## Pending impact and lead handoff

M09-06 owns clean reconfiguration and compilation on supported generators,
registered input/UV/integrated/atlas execution, Vulkan/Metal and no-RT/optional-SDK
cases, installed products/shader-only refresh, package creation and standalone
creation/update/launch. Verify copied native libraries, both paired trees and
specialization templates, same-name includes, no obsolete output in clean
artifacts, and current profiles/counters against diagnostic baselines. The added
GPU assertions may diagnose unsupported mandatory capabilities rather than skip.

M09-07 reviews the four retirement deltas and final API/products, including the
explicit coverage differences above. M09-08/M10 retain the updated packaging,
example and probe references while removing stale maintained old-example,
shader-frontend and static-library instructions elsewhere. Historical migration
reports remain evidence rather than available APIs. Pending validation, review,
documentation and distribution step files have been updated. Next step is M09-06;
this delivery stops awaiting project-lead validation.
