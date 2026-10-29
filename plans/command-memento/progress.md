# Command/memento progress and resumption

## Current status

Steps 1–3 complete: inventory/contracts, generic application core, and initial Composer command dispatch/document lifecycle integration are implemented. Rename and enabled-state edits use typed commands with undo/redo. The C++ remains uncompiled and the generic harness remains unexecuted; no Composer tests were added. Composer CMake now includes nested sources following explicit user authorization (see the linker-fix entry below); no configuration or build was run by the agent.

| Step | Status |
| --- | --- |
| 1. Inventory and contracts | Complete |
| 2. Generic application core | Complete |
| 3. Composer dispatch/lifecycle | Complete |
| 4. Optional SDK interactions | Pending |
| 5. Scene/components/selection | Pending |
| 6. Material/resources/environment | Pending |
| 7. Geometry | Pending |
| 8. Imports/baking/integration | Pending |
| 9. Documentation | Pending |

## Resume instructions

1. Read [summary.md](summary.md), this file and the first unfinished step. Read applicable `AGENTS.md` instructions and recheck for the referenced `skills.md` before engine changes.
2. Inspect `git status` and relevant source/diffs. Preserve unrelated user changes and reconcile partially implemented work; do not restart completed steps blindly.
3. Confirm the current instruction authorizes implementation. If it only requests planning or review, do not implement.
4. Follow dependencies and the agreed decisions in the summary. API names are provisional until recorded here. Do not compile or change CMake without explicit user authorization.
5. Limit automated test additions to critical generic infrastructure. Do not create Composer tests or manual application test procedures.
6. Update this file during implementation and before every handoff. If interrupted, record the last mutation, pending queued work if relevant, and the next concrete action.

## Next action

Next continuation: read [step 4](step-04-sdk-interactions.md), the inventory's step 3 checkpoint and actual dispatcher/core headers. Add optional typed SDK edit intents and explicit interaction lifecycles, preserving default SDK behavior, then connect Composer transaction wrappers. Do not compile or change CMake without explicit authorization. This turn was limited to step 3.

## Decisions to preserve

- Generic infrastructure: `bg2e::app`; Composer command grouping: `bg2e_composer::command` and subnamespaces.
- Explicit editor intentions create commands; do not observe setters to manufacture history.
- Optional selection history; no navigation or animation history; deliberate inspector/gizmo edits are recorded.
- Retain structural instances, prefer inverse geometry operations, leave global caches unchanged.
- Explicit continuous interaction transactions, with optional integration for SDK consumers.
- Final step documents actual engine APIs and SDK changes and creates the detailed `doc/architecture/command-memento/index.md` guide and linked chapters.

## Handoff log

- Planning baseline: inspected Composer action/UI/input/document and related draw/UI resource paths. `skills.md` was not found. Plan files only; no implementation started.

For subsequent entries record: step/status, files changed, actual API contracts, decisions and deviations, unresolved risks, checks performed/results, checks deliberately not run, and precise next action. Do not claim runtime correctness from source inspection.


### 2026-10-29 — Step 1 complete

- Changed: `mutation-inventory.md` (new), `step-01-inventory.md`, `summary.md`, and this progress file. Initial worktree was clean.
- Inspected Composer source responsibilities and related SDK UI, scene placement, drawable mutation, render-settings bindings, graphics queue and texture-cache contracts. `skills.md` remains absent; no engine internals changed.
- Settled proposed contracts for lifecycle/results, failure recovery and faulting, queue/history ownership, exactly-once completion, policy/origin snapshots, adoption/transactions, composite atomicity, stable save identity, nonhistorical dirty revisions, redo compatibility, limits/accounting and caller-safe release. Public API names remain proposed; no actual command APIs exist yet.
- Additional integration gaps: Graphics Settings edits scene lights/environment; post-mutation material callbacks and opaque reflection operations cannot provide undo; structural restoration needs exact placement; `releaseRetired` cleans retained nodes unless ownership is adapted; UV2 needs a prepared-publication seam; imported replacement bookkeeping must replay consistently; lightmap batches need rollback and generated-source leases; plugin callbacks/components need module leases or unload prevention; controller editing requires pause/base-state adapters.
- Chosen conflict behavior: restore authored/base values, pause the conflicting writer during an interaction, then resume it; runtime may overwrite the evaluated visible pose. Reject unsafe unsupported interactions rather than inventing history from notifications.
- Checks: source review, local Markdown link existence and whitespace checks. No runtime correctness claim. No compile, CMake configuration, compilation-backed tests, Composer tests or manual application test plans were run or added.
- Next action: step 2 generic `bg2e::app` infrastructure and focused test sources, following the inventory contracts. Steps 2–9 remain pending.


### 2026-10-29 — Step 2 complete (source implementation; uncompiled)

- Added public headers `lib/include/bg2e/app/Command.hpp`, `CommandHistory.hpp`, `CommandQueue.hpp`, `CommandTransaction.hpp`, `CompositeCommand.hpp`; implemented them in `lib/src/bg2e/app/{Command,CommandHistory,CommandQueue,CompositeCommand}.cpp`; exposed them through `app/all.hpp`.
- Actual core types: `Command`, `CommandInfo`, `CommandResult`, `CommandStatus`, `CommandState`, `CommandContext`, `CommandHistoryPolicy`, `CommandDocumentImpact`, `CommandOrigin`, `CommandCompatibility`, `CommandMemory`, `CommandResource`, `CommandReleaseSink`, `CommandReservation`, `CommandAdmission`, `CommandHistoryLimits`, `CommandDocumentState`, `CommandHistory`, `CommandTransaction`, `CompositeCommand`, `CommandQueue`.
- `prepare` allocates an owning move-only reservation/list node, freezes effective policy/context and budgets retained resources before publication. Execution/begin recheck memory estimates before mutation. Recorded success transfers the already allocated node into history; no-op/rejection/recovered failure preserves cursor, state and redo. Shared immutable resource/version tickets are counted once with consistent-size validation. Pending reservations, undo/redo and active previews share limits.
- Limits deliberately reject admission/growth instead of automatically evicting old entries. `discardOldest`, `discardRedo` and `setLimits` provide explicit stable-boundary policy; saved identity survives pruning. Deferred retirement memory belongs to the application's release service, outside the history's retained budget.
- Commands implement `execute`, `undo`, `redo` as `noexcept`, including their own partial-failure recovery. Results carry allocation-free static-lifetime diagnostic views. Composites recover completed children in the appropriate order; failed compensation retains the composite and faults history. Repair/replacement requires a new-generation `resetBaseline`, not a cleared error bit.
- A transaction is an undoable `CommandTransaction` subclass implementing `begin`, `memoryForUpdate`, `update` and `changed`. History owns one active reservation. Begin captures before once, updates reserve before mutation, commit adopts without execute, cancel restores before, unchanged commit preserves redo. Recovered cancel failure leaves the preview active for retry. `transactionId` guards queued operations against replacement previews.
- Document state uses unique state IDs plus generation and nonhistorical revision; selection-only entries retain document state. `markSaved` is a post-success-IO operation after queue flush/preview settlement. Nonhistorical compatibility is explicit (`Preserve`, `InvalidateRedo`, `ResetHistory`); preferences only resolve `UserPreference` at admission and never filter existing entries.
- Queue admission and completion are distinct. The injected executor schedules one head at the caller's safe boundary, with run/cancel callbacks and exactly-once completion. Call `pump` each owner tick; delayed completion makes the next head ready for the next pump. Undo/redo and transaction begin/update/commit/cancel are explicit queue operations. Ordinary updates preserve FIFO; only adjacent unscheduled absolute updates matching interaction, generation and target/property coalesce, with superseded updates completing NoOp. Queue payloads have an estimated CPU byte limit and a 1024-job cap.
- Release sinks receive both command ownership and retained resource tickets, enabling caller-safe resource/module retirement without core GPU dependencies. Owner-thread-only calls are enforced; async producers must marshal. Queue shutdown restores its owned preview and cancels pending work; application owners must remain live through successful settlement/release.
- Added `tests/app/command_memento/main.cpp` and its API/source-harness README. Behavioral sources cover ordering/completion/cancellation, policy snapshots, branching/no-op/failure cursor atomicity, fault recovery, composites and compensation faults, preview adoption/cancel/growth, selection versus document tokens, saved-state eviction, unrecorded compatibility, shared budgets/retirement, stale/foreign reservations, owner-thread checks and absolute coalescing boundaries. No framework or CMake integration added.
- Checks performed: source review; textual include/dependency, license, whitespace and delimiter checks for all ten new C++ files; local Markdown link existence; `git diff --check`. These checks passed and do not establish compilation/runtime correctness.
- Checks not run: compilation, CMake configuration, compilation-backed tests and Composer/application verification, per repository and plan instructions. `skills.md` remains absent after rechecking. No scene/UI/draw/render/gpu/Composer source changes.
- Remaining integration contracts: step 3 must funnel Composer execution/replay through one queue, pass current generation, flush before save/replacement and provide a resource-safe release sink. Later SDK/structural/resource adapters still need the gaps listed in step 1; no integration is implied by the generic core.
- Exact next action: inspect `step-03-composer-dispatch.md`, `Document.hpp`, `AppDelegate.*`, `DocumentIo.cpp`, and the new command headers, then implement Composer dispatch/lifecycle and initial commands within the next authorized step. Steps 3–9 remain pending.


### 2026-10-29 — Step 3 complete (source integration; uncompiled)

- Added `apps/bg2e_composer/src/CommandDispatch.cpp`, `command/Dispatcher.hpp/.cpp`, and `command/scene/NodeCommands.hpp/.cpp`. Updated Composer delegate, Document, document IO, StageScene, scene editor, action notifications, material guards, import rebind, rendering fault guard and initial Edit menu entries.
- Actual application APIs: `command::Dispatcher`, `command::Boundary`, `command::Effects`, `command::scene::RenameNode`, `command::scene::SetNodeEnabled`; delegate methods `renameNode`, `undoEdit`, `redoEdit`, safe scheduling/effects/flush helpers; `Document::commands` and `synchronizeDirty`; `StageScene::EditingState` capture/restoration and read-only graph validation mode.
- Commands freeze weak target identity/document generation at dispatch, validate at execution/replay and capture before values at the safe boundary. Rename uses CPU-value updates; enabled state and history replay use conservative updates. The graphics adapter includes a retained-resource route and category boundary validation for later resource commands. Only the current CPU-state/weak-identity command families are integrated; native-resource mementos still require their later lifecycle adapter.
- Dispatcher envelopes apply centralized effects inside the recoverable operation, compensate the model on effects failure and report Faulted on failed compensation. Selection-only effects do not automatically dirty/invalidate the document. Enabled snapshots preserve camera/environment/fill/known-environment references and controller identity; unsupported runtime writer conflicts still need step 4 adapters.
- History ownership lives in `Document::commands`; the UI-facing dirty bool is synchronized from stable state/save IDs plus nonhistorical revisions. Legacy authored notifications explicitly reset incompatible history until migrated. Navigation never records and uses Preserve revisions, including continuous keyboard navigation detected through orbit-state sampling. Sampling is reset after explicit edits/save to avoid marking saved navigation dirty on the following frame.
- Save/open/new/export/close flush serialized commands and settle active previews before evaluating dirty state or writing. Save rejects unsettled/faulted history and marks saved only after complete successful IO; paths stay outside mementos. Faulted history can still be discarded via close/replacement.
- Replacement prepares a new-generation dispatcher before publication and retains the old document/history through graph/renderer reattachment. Failed publication restores original stage order, active references and selected identities; failed recovery preserves recovery resources and faults the history. Tentative import rebind now emits effects without prematurely clearing the old history.
- Added generic `CommandHistory::reportExternalFault()` because renderer/document publication recovery happens outside command execution. Added a focused assertion sequence to the existing generic test source for fault blocking and reset; it was not compiled/run. No Composer test sources or manual application test plans were added.
- Retirement ownership is transferred without allocation through preallocated command envelopes/intrusive links and drained at owner-safe boundaries. Current commands retain no native realizations, so renderer recreation preserves their CPU mementos; pending work still obeys renderer-generation cancellation. Queue/history callbacks are retired before tools, renderer, stage and engine teardown. GPU-bearing commands/module leases remain later-step work, explicitly tracked in the inventory.
- Minimal Undo/Redo Edit menu routes (Ctrl+Z / Ctrl+Shift+Z) expose the first commands; full history preferences/menu behavior remains step 8. Rename text updates are discrete until explicit SDK interaction boundaries arrive in step 4.
- Remaining legacy paths and compatibility limitations are enumerated in `mutation-inventory.md`: component/reflection, structure, metadata, geometry, material/texture/environment, camera intrinsics, gizmos, graphics scene settings and prepared result publication. Void/post-mutation legacy notifications cannot yet distinguish every no-op/recovered failure and may conservatively discard undo/redo. They do not become opaque command wrappers.
- Checks: reviewed execution/replay, generation/cancellation, effects recovery, saved identity, replacement rollback and teardown source paths; textual GPL/include/delimiter checks across 32 Composer/core source files; whitespace and Markdown link checks. These are source checks only and do not establish compilation or runtime correctness.
- Not run: compilation, CMake configuration, compilation-backed tests, Composer tests or application verification. `skills.md` is still absent. No `bg2e::render`, `bg2e::gpu`, global texture cache or CMake changes.
- Exact next action: read `step-04-sdk-interactions.md`, `command/Dispatcher.*`, `CommandDispatch.cpp`, `DrawMaterialEditor`, `ReflectionWidget`, `ComponentInspector` and gizmo interaction sources, then implement optional typed intent/transaction seams. Steps 4–9 remain pending.


### 2026-10-29 — Composer nested-source linker fix

- The user-provided build log reached the Composer link stage but omitted `command/Dispatcher.cpp` and `command/scene/NodeCommands.cpp`, causing undefined symbols. The shared `bundle_app_sdl` helper uses a nonrecursive `src/*.cpp` glob; the initial integration incorrectly assumed nested application sources were included.
- The user explicitly authorized changes to the application's CMake files. Updated only `apps/bg2e_composer/CMakeLists.txt` to recursively collect C/C++ sources and headers with `CONFIGURE_DEPENDS`, remove sources already supplied by the helper, and append the remainder with `target_sources`. Command implementations remain in their intended subdirectories; the shared helper and other applications are unchanged.
- Checks: reviewed the helper's source collection and resulting application target setup; `git diff --check` passed. No CMake configuration, compilation or test execution was performed by the agent. The supplied build log demonstrates a link failure, not successful build/runtime verification.
- Next action: reload the CMake project and rebuild Composer to verify linkage. The next implementation step remains step 4.
