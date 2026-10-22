# Application packages

Windows (run from a Visual Studio 2022 developer PowerShell with CMake on PATH):

```powershell
.\scripts\package_windows.ps1 -Configuration Debug -Destination C:\packages
.\scripts\package_windows.ps1 -Configuration Release -Destination C:\packages\release -VulkanSdk C:\VulkanSDK\1.4.xxx.0
```

macOS and Linux:

```sh
bash scripts/package_macos.sh Debug /existing/package/directory "$VULKAN_SDK"
bash scripts/package_linux.sh Release /existing/package/directory "$VULKAN_SDK"
```

Configuration and destination are mandatory; there is no implicit Release default.
Use the CLI supported by the selected platform:

| Platform | Script and generator | Archive | Optional behavior |
|---|---|---|---|
| Linux | package_linux.sh; Ninja | `.tar.gz` | `--allow-dirty` and `--overwrite` before positional arguments. |
| macOS | package_macos.sh; Xcode | `.zip` | No dirty-check bypass or overwrite flag. |
| Windows | package_windows.ps1; Visual Studio 17 2022, x64 | `.zip` | Optional `-VulkanSdk`; no dirty-check bypass or overwrite switch. |

All platforms require initialized, matching submodules. Clean-checkout validation
includes staged, unstaged and untracked changes, excluding ignored files. Linux
can explicitly bypass the working-tree check with --allow-dirty; its package name
and build-info still identify HEAD and do not encode uncommitted modifications.
A packaging instruction is not an instruction to commit unrelated work.

Destinations must be outside the repository. Linux/macOS require an existing
destination directory; Windows creates its package directory under the supplied
external destination. Existing folder/archive paths are rejected except for
Linux's explicit --overwrite, which removes those named outputs before building.

Each invocation retains a fresh temporary build tree for diagnostics and builds
bg2e_distribution with examples/tests OFF and BG2E_PACKAGE_APPS ON. No previous
bin/ binaries or stale shaders are package inputs. CMake recursively collects
executable targets under apps/: model_edit, bg2e_composer and lightmap_generator.
Their presence is checked after Distribution install. The camera export tool is
an example and is not included by these default application-package scripts.

The unpacked folder is bg2engine_<12-character-or-longer-local-HEAD-hash>.
build-info.json records the full commit and configuration. Linux emits a matching
.tar.gz; macOS and Windows emit .zip. Use distinct destinations for Debug/Release
of the same commit. Archives contain the named folder. No fetch or remote checkout
occurs. Dirty Linux builds do not make the commit a complete input identifier.

Linux stages install/archive in the temporary tree before moving them to the
destination. It removes top-level external .so files copied by CMake, retaining
only libbg2e.so variants. Target hosts must provide compatible external graphics,
SDL/desktop and C++ runtime libraries; the executable-relative RPATH locates the
engine, not a bundled distribution of the host system stack. macOS/Windows install
directly into the destination package; failure can leave partial output to inspect.
These procedures do not promise atomic package replacement or power-loss durability.

macOS retains the existing app bundle Vulkan/MoltenVK/SDL/resource rules and
project-disabled code signing; it is not a signed distribution. Windows packages
executables, engine DLL, resolved non-system imported DLLs, compiler redistributable
runtime, shaders/assets and Debug PDBs. Import/static libraries and build
intermediates are excluded. The engine uses /MD in both configurations, so Debug
symbols do not imply a debug CRT. Binary dependency scanning cannot discover every
dynamically loaded plugin/layer; compatible driver/runtime deployment still matters.

All scripts guard against a changed HEAD during the build. macOS/Windows and clean
Linux builds also recheck the working tree. Native package/relocation validation
remains project-lead work; source inspection is not a package execution result.

## Production draw/bake shader resources

The engine also installs the paired native `shaders/draw` and `shaders/bake`
trees through the RuntimeShaders component. Preserve their relative paths in
packages and precompiled-library projects; stage filenames are not globally
unique. The paired aggregate compiles both source trees and application resource
refresh copies both even on shader-only rebuilds. On macOS the active bundle must
contain `bake/uv_surface/surface.vert.metallib`; Vulkan uses the matching `.spv`.
After updating an older checkout with missing bake registration, reload CMake
and rebuild the application, then use its refreshed bundle. See
[shader deployment](../doc/api/gpu/ShaderLibraries.md#engine-paired-drawbake-deployment)
and [draw_lightmaps](../doc/apps/draw_lightmaps.md#build-and-resources). Resource deployment itself does not execute baking or export. The delivered
[bake scheduler/writer](../doc/api/bake/StandaloneBakeBatch.md) and camera CLI
provide those distinct application workflows.

The [draw offscreen CLI](../doc/apps/draw_offscreen_application.md#build-and-resources)
is registered as `draw_offscreen_application` using the standard example resource
helper. Keep native draw shaders, scene assets and platform runtime libraries
with its executable/bundle. It writes one completed camera PNG; the common resource
packaging helper does not itself implement scene-image export or standalone bake
batch output. Packaging/install/standalone coverage remains subject to the actual
selected script component and target configuration; registration alone does not
establish that every distribution variant contains the example.


## Draw standalone lightmap generator product validation

`lightmap_generator` is an application target under `apps/lightmap_generator`. Standard
bundle_app rules supply native libraries/assets and paired draw/bake shader trees;
recursive Distribution discovery includes it alongside `model_edit` and
`bg2e_composer`. RuntimeShaders installs both paired trees. No special package
script or alternate shader layout is needed for the new CLI.

Registration is source evidence only. On actual supported hosts validate fresh
and shader-only builds, active bundle resources, installed Distribution and
RuntimeShaders contents, and a relocated headless job. In particular inspect
bake/uv_surface/surface.vert.metallib on Metal and its .spv peer on Vulkan at the
**executed** shader root, not just the build source tree. Record backend/device,
exact package/component/command, exit code and exported artifacts.

Successful jobs publish lightmaps.json pointing to a complete unique output
generation. Preserve that layout and independent native model sidecars when
moving job output; these outputs are not engine runtime assets. The read-only
[independent artifact inspector](../tests/bake/product/README.md) checks PNG/native
HDR maps, metadata and model material sidecars; draw engine geometry reload and
other image decoders remain separate lead checks. See the
[M08-05 evidence matrix](../docs/render_to_draw/plan/milestone_08_lightmap_generator/product-validation.md).
No backend/package success or measured performance is implied by registration or
synthetic inspector tests. Package scripts retain their existing clean-checkout,
destination and platform-generator requirements.

## Canonical renderer and standalone refresh

The engine now builds and deploys only the paired `shaders/draw/` and
`shaders/bake/` libraries. Native Vulkan/Metal SDK files, optional FidelityFX
linkage and SMAA tables/licenses remain required. Engine shader compilation uses
separate tree targets with the shared `bg2e_paired_shaders` aggregate; app bundles
refresh both trees on shader-only rebuilds. Flat legacy shaders and old example
sources/compiler frontends have been retired. Reconfigure existing build trees
before building after this source removal.

Applications live directly in `apps/bg2e_composer`, `apps/model_edit` and
`apps/lightmap_generator`. Packaging scripts verify that all three installed
executables are present (inside their app bundles on macOS).

New packages collect the three registered `model_edit`, `bg2e_composer`
and `lightmap_generator` products. Their IDs/preferences/server behavior are
unchanged. Non-Apple install rules copy the authoritative assets and paired engine
trees, plus any target-specific shader directory; stale flat build outputs are
not engine distribution inputs. Apple retains native bundle/SDK resource rules.

Standalone creators copy canonical headers, the deferred draw template and only
the two paired engine shader trees. They require representative G-buffer and UV
surface SPIR-V files; macOS also requires their Metal libraries. macOS library
lookup accepts the root output or Xcode Release/Debug output (root first, then
Release, then Debug). Rebuild current sources before creating a project: these
checks establish file presence, not binary/header ABI compatibility.

Both library updaters now refresh headers, the current-platform library, complete
draw/bake shader trees and standalone_utils.cmake together. The shader source is
PRODUCT_DIR/shaders even with multi-configuration binary subdirectories. draw/
and bake/ are engine-owned and replaced; custom application resources outside
those two trees are preserved. Historical flat/custom files in an existing
standalone project are left alone and no longer bundled as engine shaders.
The update is not transactional: a copy failure can require rerunning it. Existing
build-producing behavior is unchanged; do not execute an updater merely to inspect
its source. Build/package/standalone launch validation remains project-lead-owned.

## Standalone shader preflight

The shell and PowerShell creators/updaters require the canonical paired stages
`draw/raster/gbuffer.frag.spv` and `bake/uv_surface/surface.frag.spv`. Shell scripts
also require their `.metallib` counterparts on macOS. Creators check these files
before writing the project; updaters check them after building and before copying.
Build current sources first. File presence does not establish binary/header ABI
compatibility or successful standalone execution.
