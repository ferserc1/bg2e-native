# Phase 4: Document the plugin API and tutorial

Depends on phases 1–3. Write all documentation in English and describe the implemented public API, not speculative signatures.

## 1. API documentation

- Create `doc/api/plugin/` following the style and navigation conventions of existing `doc/api/` modules: an index, practical quick start, reference and per-type/topic pages as appropriate. Include header and namespace annotations, signatures, contract tables, examples and related links.
- Document Plugin, PluginManager/facades, contexts, reports/errors, handles, ComponentRegistrar, metadata/license fields and the declaration macro. Explain which code authors write and which exports and compatibility values the SDK generates.
- Explain contract C version, SDK version/ABI identity and module ABI configuration separately from plugin version. Describe conservative matching, useful mismatch diagnostics, compatible toolchain/runtime requirements and the limits imposed by native loading/static initialization.
- Cover automatic discovery and explicit file/directory loading, default paths, scan ordering, non-plugin handling, duplicate registration reporting, reflection/subtype ownership, lifecycle/service availability, recoverable failures and the programmer-owned unload contract in `doc/api/plugin/unloading.md`. Explicitly require scene cleanup for every root and release of all external objects, weak control blocks and executable references; do not promise complete automatic lifetime tracking or permanent retention of component modules.
- Explain shared-engine linkage and platform dependency resolution, with clear CMake helper arguments and examples for repository and standalone applications. Explain the generated empty standalone plugin and how to add components.
- Update `doc/api/index.md` and relevant existing API cross-links, including PlatformTools, without unrelated documentation changes.

## 2. Step-by-step plugin tutorial

- Write `doc/tutorials/plugins/plugin.md` for `examples/tutorials/plugins/`, linked from its README and applicable tutorial navigation.
- Introduce the reflection tutorial as the prerequisite and reuse its explanation of scene creation and component definitions. The existing files are `doc/tutorials/refection/01_scene_and_manipulation.md`, `02_rotation_component.md` and `03_reflection_editor.md`; link using their real paths and avoid an unrelated directory rename.
- Explain the resulting scene and preserved engine inspector briefly. State that the application-owned manual reflection inspector is omitted because this tutorial focuses on plugins.
- Walk through application/module source organization, moving both components and supporting reflection definitions into the plugin, implementing the plugin class and metadata, using the declaration macro and explicitly registering contributions through the registrar.
- Show the reusable CMake helper invocation, linking to the same shared engine, and platform deployment destinations. Explain that the helper places the module in `plugins` (inside bundle Resources on macOS) and MainLoop discovers it automatically before scene initialization.
- Explain host creation of the initial Rotation component through the registry and normal engine-inspector discovery of the plugin components, without concrete component implementations linked into the host.
- Mention only that explicit plugin loading is also available, linking to the API documentation. Do not include explicit-load instructions, code or an alternative explicit-loading walkthrough in the tutorial.
- Explain lifecycle ownership and required release of application references where relevant to the example. Preserve its serialization policy and avoid unrelated scene/reflection explanations.
- Do not add build instructions, compilation steps, tests or runtime verification exercises as part of this implementation plan or its new documentation deliverables.

## Completion review

Review documentation against the implemented source, existing documentation style, actual paths and relative links. Do not add or execute tests, builds, configuration commands or application launches. Update `progress.md` with the last implemented step and stop for user feedback.
