# Standalone scene assembly manual verification

`runStandaloneAssemblyProbe` in `probe.hpp` takes a stopped initialized **offscreen**
draw Engine and a fresh disposable directory. It writes probe BG2/JSON files and
modifies only those fixture files; it does not run a bake, final export or cleanup
of the caller's directory. All assemblies/GPU owners must die before Engine cleanup.
There is no CMake target or new test framework. The agent has not compiled/run it.

The fixture checks camera-less context, CPU target mode and explicit later loading,
BG2 joints/material data, UV2 metadata and unchanged input bytes, multi-root retention,
single serialized Node adaptation, stable distinct output identities across fresh
runtime IDs, helper exclusion, unknown/null entries continuing to valid components,
registered parser failure, candidate isolation, independent CPU/GPU clones and
nonfatal real UV2 overlap. The fixture uses standard shared JSON/schema/DB paths.

Lead checks on available Metal/Vulkan:

- Compile library and fixture in the usual harness. Assemble model/prefab with and
  without generated UV2; feed the loaded assembly root and each target to M08-01,
  releasing each baker before the next and delaying map adoption until all finish.
- Verify context geometry occludes/illuminates but is never selected as a target;
  enabled target-tree lights and Environment selection survive attachment. Accept
  camera-less scenes. Check all authored transforms and chains, HDR/procedural
  environment paths, exact custom schema/resource/clone hooks and extra components.
- BG2/VWGLB and OBJ/MTL standard Drawable references reuse CPU parsers. Check texture
  paths, native archives, groups/names/visibility and authored scalar/material values.
  No glTF-hierarchy-to-single-Drawable conversion is supplied by this assembler.
- Disabled branches stay in the graph/dependency list but are not selected targets.
  Empty/disabled-only prefabs, missing assets/textures, invalid structural/finite UV
  or normal data, singular/nonfinite transforms, transparent receivers and excessive
  enabled lights must fail explicitly. Genuine overlaps warn and stay eligible.
- Test two independently loaded objects referencing the same input asset: mutable
  materials/textures and UV2 replacements must remain independent. Compare source
  files before/after success and later-target failure. A failed candidate is not
  returned and must not change an existing Assembly/context or create output files.
- `loadTargetGpuResources=false` defers target-tree resource hooks. Context sources
  are still realized. Standard targets have CPU meshes/material references and can
  use `targetRoot->loadResources(engine)` later. A failure in that explicit later
  operation can leave an incomplete private candidate: discard it before baking.
- Custom Drawable resource formats use their virtual loader in immediate mode;
  CPU mode requires that graphics-free deserialization already supplies a CPU draw
  mesh. Custom resource hooks must not replace the prepared UV2 target during final
  realization. Check failure remains explicit and does not fall back to render.
- Inspect `sourcePaths` canonical standard geometry/texture/environment/input files,
  source alias/symlink handling, deterministic sanitized hierarchy paths and sibling
  collision resistance. Custom resources and OBJ MTL source-file discovery require
  supplemental protection in M08-03; these lists do not constitute a batch preflight.

Assembly does not allocate an environment generator, build a context TLAS, decide RT
availability, publish lightmaps or implement protected batch output/CLI. Those have
separate owning steps; no native runtime/backend evidence is implied here.
