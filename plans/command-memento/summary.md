# Command/memento implementation plan

Status: planning complete; implementation has not started. This plan does not authorize compilation or CMake changes.

## Purpose

Organize Composer scene actions as identifiable command objects and provide undo/redo, while making the generic infrastructure reusable through `bg2e::app`. Keep ordinary SDK widgets, components and manipulation tools usable without commands or a history.

All generated files and documentation must be in English. Follow repository instructions, including source license/style requirements. Do not modify `bg2e::render`, `bg2e::gpu` or unrelated systems. Composer currently uses `bg2e::draw`.

## Agreed design

- Generic command, queue, history, transaction and composite infrastructure belongs directly to `bg2e::app`, not an engine `command` namespace. Names in this plan are proposed until implemented.
- Composer commands belong to `bg2e_composer::command`, with subnamespaces/directories for scene, component, material, geometry and lighting actions.
- Replace opaque pending scene-edit lambdas with command instances. A small transport lambda inside an existing graphics queue is acceptable; it must not remain the owner of undo semantics or hide an unclassified edit.
- Separate the execution queue from executed history. Record only successfully completed, effective edits. Support adoption of an already-applied interaction without executing it twice.
- One command interface serves discrete and continuous edits. Interaction transactions provide begin/update/commit/cancel; explicit interaction boundaries are preferred over debounce.
- Commands expose undo capability, category, history policy (`Never`, `Always`, `UserPreference`) and document impact. Composer resolves category preferences and permitted origin into an effective recording decision before execution or at transaction start. Invalid combinations must be rejected, not silently recorded.
- Selection can be undoable according to user preferences, without making the scene dirty. Preferences affect future actions only. Navigation is never recorded, even when preferences change.
- Editor-originated edits are explicit commands. Runtime animation, component updates and camera navigation do not automatically become commands. Setters and generic change notifications must not create history entries. Undo/redo never recursively records itself.
- Keep navigation outside undo/redo. Editing a camera's properties or node deliberately through the inspector/gizmo is an ordinary scene edit. Persisted navigation changes, if retained by current serialization, require separate dirty tracking.
- Retain component/node instances for structural undo; do not depend on serialization. Preserve identity, sibling/component placement and active-scene references.
- Prefer reversible geometry operations with stored forward/inverse operations. Use retained old/new geometry data or buffers for information-losing operations. Do not recompute redo from current bounds or selection.
- Do not modify the global texture cache or introduce a texture manager. Retain command resource references and temporary/generated sources as needed; respect device-bound lifetimes and GPU retirement.
- File open/save/export and parsing stay outside commands. Applying prepared imported content, material patches, environment presets or baking results is a scene command.
- Command failures must not advance history. Prepare before publication and roll back partial edits. A failed recovery enters an explicit fault state rather than pretending consistency.
- Maintain history memory limits, correct saved-state identity, and separate document changes from optional selection entries. New recorded edits discard redo; unrecorded actions need an explicit compatibility/invalidation policy.

## Steps

| Step | Deliverable | Dependencies |
| --- | --- | --- |
| [1. Mutation inventory and contracts](step-01-inventory.md) | Source-backed action classification, ownership and integration contract | None |
| [2. Generic application infrastructure](step-02-app-core.md) | Command objects, history, queue, transactions, policies and focused core tests | 1 |
| [3. Composer dispatcher and document lifecycle](step-03-composer-dispatch.md) | Safe execution, generation guards, dirty/save semantics and first commands | 2 |
| [4. Optional SDK edit interactions](step-04-sdk-interactions.md) | Typed edit intents and optional UI/gizmo interaction lifecycle | 3 |
| [5. Scene, selection and component commands](step-05-scene-components.md) | Structural commands, retained instances and configurable selection history | 4 |
| [6. Material, texture and environment commands](step-06-material-resources.md) | Continuous property edits and resource-safe restoration | 5 |
| [7. Geometry commands](step-07-geometry.md) | Inverse operations and fallback retained geometry | 6 |
| [8. Imports, baking and complete integration](step-08-integration.md) | Prepared-result adoption, menus/preferences and mutation coverage | 7 |
| [9. API, SDK and architecture documentation](step-09-documentation.md) | Final references and detailed multi-file technical guide with real code excerpts | 8 |

## Source baseline

- `apps/bg2e_composer/src/EditorActions.cpp`: node-edit queues, geometry operations, material paste/patch, environment actions and invalidation.
- `SceneEditor.cpp`: reflected edits, component operations, node creation and reparenting.
- `EditorTools.cpp`: material-editor binding and submesh metadata edits.
- `AppDelegate.cpp`: queued input, selection, gizmo integration and navigation-driven mutations.
- `Document.hpp`, `DocumentIo.cpp`, `ImportActions.cpp`, `EditorBaking.cpp`, `StageScene.*`: document identity, save/import/publication, baking and scene ownership.
- `lib/include/bg2e/ui/{DrawMaterialEditor,ReflectionWidget,ComponentInspector}.hpp` and implementations: optional intent handlers exist, but material notification is post-mutation and reflection currently passes opaque operations.
- `lib/src/bg2e/draw/Drawable.cpp`: modifiers mutate CPU geometry and rebuild GPU representations.
- `lib/include/bg2e/utils/TextureCache.hpp`: weak device-specific texture realizations; leave cache management unchanged.
- Repository instructions reference `skills.md`; it was not found during planning. Recheck before engine implementation and record if still absent.

## Scope of checks

Do not add manual test plans, Composer tests, application verification scenarios, or a testing phase. The user will exercise Composer; application tests belong to a future plan. Focused automated tests under `tests/` are permitted only for critical generic infrastructure, as specified in step 2. Do not compile, configure CMake or run compilation-backed tests without an explicit user request. Source review, Markdown link checks and whitespace checks are allowed; report them accurately.

## Resuming and maintaining progress

Before any implementation or continuation, read this file, [progress.md](progress.md), the current step, and applicable repository instructions. Reconcile the recorded state with the actual worktree before changing anything. Read referenced source contracts rather than assuming proposed API names already exist.

Execute steps in dependency order within the scope authorized by the current user instruction. This document itself is a plan, not an instruction to begin implementation. Do not introduce a new approval requirement between otherwise authorized steps.

Update `progress.md` after each completed step and before stopping or handing off. Record completed/partial work, actual API names, files, design decisions, unresolved issues, allowed checks performed, checks not run, and the exact next action. Keep step status current. Do not mark a step complete when its required deliverables remain missing. The final documentation step must describe implemented behavior with excerpts from actual code, not this proposal.
