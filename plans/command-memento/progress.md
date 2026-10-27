# Command/memento progress and resumption

## Current status

Planning only. No implementation, API changes, tests or architecture documentation have been created. No build or CMake configuration has been performed.

| Step | Status |
| --- | --- |
| 1. Inventory and contracts | Pending |
| 2. Generic application core | Pending |
| 3. Composer dispatch/lifecycle | Pending |
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

When implementation is requested, start [step 1](step-01-inventory.md): inventory all scene mutation entry points and settle queue/history, origin, ownership and dirty-state contracts before coding the reusable core.

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
