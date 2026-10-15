# Paired native shader deployment (M01-04)

## Loading contract

`gpu::ShaderLib` is a naming/loading convenience. Construct it with the actual resource directory and backend; it borrows neither a device nor a shader module. Each load returns a shared module owned by its caller. Pipeline descriptions borrow those modules, so keep them alive through pipeline creation/use as required by the GPU contracts. No cache, device resource registry, translated shader source or inferred binding mapping is introduced.

For example, `ShaderLib(resourceRoot / "shaders/draw/composite", backend).fragment("probe", device)` opens `probe.frag.spv` on Vulkan, or `probe.frag.metallib` on Metal. A root library may equivalently load `draw/composite/probe`. Relative directory components are preserved; absolute names and `.`/`..` components are rejected. `stagePath(name, stage)` resolves without opening a file; `entryPoint(backend, stage)` exposes the actual convention. Stages are vert, frag, comp, rgen, rmiss and rchit. Vulkan uses `main`; Metal uses `<stage>Main`. Optional Metal miss/hit loading retains its historical missing-file behavior. Required stages fail through the native loader.

The old header incorrectly claimed all entries were `main`. Historical `compile_shaders_shaderlib` passes `-e <stage>Main`; this step leaves that helper and existing example paths intact and adds a separate helper that unambiguously keeps GLSL `main`. Native stages remain explicit and independent; this is no new authoring or binding abstraction.

## Source and output tree

- Sources: `shaders/src/draw/<pipeline>/<name>.<stage>.glsl` and `.metal` together.
- Shared backend code: `shaders/src/draw/lib/{math,pbr,color}.{glsl,metal}` with include guards and native language types. Math covers radical inverse/Hammersley/GGX sampling; PBR covers Fresnel, GGX distribution, geometry and direct BRDF; color covers gamma and exposure/brightness/contrast. Helpers do not include scene/material binding structures. Future steps add those explicitly. Positive roughness/normalized, nondegenerate vectors are the PBR caller contract. Numerical guards bound singular denominators.
- Build outputs: `${PRODUCT_DIR}/shaders/draw/<pipeline>/<name>.<stage>.spv`, plus `.metallib` on macOS. AIR intermediates remain under the build directory, outside runtime resources.
- Pipeline names are retained, so `gbuffer/probe.vert` and `composite/probe.vert` cannot overwrite each other.
- `gbuffer/probe` and `composite/probe` are deployment checks, not the final G-buffer or deferred-lighting programs. Their vertex stages generate a fullscreen triangle without geometry resources. Fragments produce distinct constant BRDF/color-corrected outputs in a single linear RGBA8 attachment. Composite also provides a bounded image-fill compute kernel (16x16 threadgroups) using Vulkan set 0/binding 0 and Metal texture 0, with RGBA8 storage output. No automatic binding mapping is involved.
- M01-08/M01-10/M01-11/M01-13 still implement their actual environment, six-channel G-buffer/composite, query and upscaling libraries. The probe does not establish those future contracts.

## Build and deployment integration

`compile_paired_shader_tree` in `cmake/utils.cmake` recursively collects the native stages, preserves paths and builds both backends on Apple (Vulkan only elsewhere). It uses Vulkan 1.2, includes the draw source root, and depends conservatively on every shared `.glsl`/`.metal` include. New files are tracked with `CONFIGURE_DEPENDS`. Existing production compilation remains separate and unchanged. `lib/CMakeLists.txt` registers this tree as a dependency of bg2e; library-only builds therefore produce the engine shaders.

Existing app helpers recursively bundle `BG2E_SHADER_DIR`. A draw-resource dependency refreshes paired outputs even when shader-only edits do not relink the app; single-config Linux/Windows already use the same runtime directory and need no duplicate copy. macOS uses `Contents/Resources/shaders/draw`, other app outputs use `shaders/draw` beside the executable. Multi-config output locations are handled via target-directory expressions.

Direct installation includes `shaders/draw` in component `RuntimeShaders`. Existing `Distribution` app packaging already installs complete resource directories (or complete bundles on macOS), including this tree. Standalone creation scripts now copy the entire shader tree, preserving `.spv`, `.metallib` and pipeline directories; the existing standalone app helper recursively bundles it. These scripts were edited, not executed. The library-update scripts remain library/header updaters; regenerating a standalone project refreshes its shader snapshot.

## Validation handoff

Agent checks: native signatures/call sites, stage names, explicit bindings, paired file/output uniqueness, local includes and shared dependency discovery, packaging/copy paths and `git diff --check`. No configuration, compilation, shader compilation, execution, installation or packaging was run.

Project lead checks:

1. Build bg2e on available Vulkan/Metal platforms. Confirm both pipeline folders contain separate vertex/fragment outputs and composite has compute output. Production shader files and existing examples must still load.
2. Compile the manual MRT runner and pass the deployed engine shader directory as its optional third argument; see the [shader library probe](../../../../tests/gpu/shader_libraries/README.md). This creates pipelines, renders both libraries and dispatches compute, then verifies readback; no RT or FSR required.
3. Edit a shared include, rebuild the application target and confirm dependent shader outputs and macOS/multi-config app resources refresh even without C++ changes.
4. Repeat the probe against installed `shaders` directories, packaged app resources and a generated standalone project's shaders. Confirm stage names and distinct pipeline paths survive deployment. Verify existing production application behavior independently.

Current backend/build/deployment coverage is unspecified until lead validation. M01-05 is the next implementation step after acceptance.
