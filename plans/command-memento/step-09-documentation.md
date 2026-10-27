# Step 9 — Engine API, SDK changes and detailed architecture documentation

Dependency: steps 1–8. This must remain the final implementation step.

## Engine and SDK references

- Document every new public `bg2e::app` command/history/queue/transaction API in the existing API-reference structure, with actual declarations, ownership, defaults, results, failure behavior, threading and lifetime requirements.
- Document all changed public SDK surfaces: UI edit intents, reflected property paths/values, material editor integration, gizmo lifecycle hooks and any geometry/resource restoration helpers actually introduced.
- Explain optional integration and compatibility for existing applications, including examples with no history. Explicitly distinguish notifications from mutation requests and supervised already-applied edits.
- Update API indexes and relevant existing UI/manipulation/application pages. Document migration from previous callback contracts where applicable. Do not invent signatures or claim runtime verification.

## Required multi-file technical guide

Create `doc/architecture/command-memento/`, indexed by `index.md`. Add its entry to `doc/architecture/index.md`. The guide is a detailed implementation explanation, not just an API listing or design-pattern summary. Assume only C++ knowledge: explain scene/component ownership, application frames, UI callbacks and GPU-derived state before relying on them.

Required chapter coverage (filenames may be adjusted to the final implementation):

| File | Content |
| --- | --- |
| `index.md` | Reading order, vocabulary, module map, source map and implemented scope |
| `foundations.md` | Command and memento roles, state versus intention, queue versus history, and why setters do not record |
| `app-core.md` | Actual classes, ownership, results, state transitions, policies, categories, composites and fault behavior |
| `execution-and-history.md` | Dispatch ordering, safe execution, already-applied adoption, cursor/branching, save identity, dirty state, limits and eviction |
| `continuous-interactions.md` | Begin/update/commit/cancel, initial capture, coalescing versus grouping, compound controls, invalid inputs and cancellation |
| `sdk-integration.md` | Optional UI/reflection/material/gizmo interfaces, integration-free use, source-compatible migration and lifetime contracts |
| `composer-flow.md` | Exact Composer action-to-command-to-renderer-to-history path, contexts, generations, effect reporting and menu/preferences |
| `composer-scene-and-components.md` | Selection policy, retained structural instances, parent/order restoration, properties and active scene references |
| `composer-geometry.md` | PutOnFloor's stored matrix/inverse, full data affected, floating-point limits, shared geometry and irreversible fallback |
| `composer-resources.md` | Materials, textures, environments, imports, UV/lightmap adoption, GPU rebuild/retirement and temporary resource retention |
| `origins-and-lifecycle.md` | Runtime animation/navigation exclusions, deliberate camera editing, controller conflicts, nonhistorical dirty state, document replacement and plugin/engine teardown |
| `extending.md` | How to add a command/category/property adapter or integrate another application, with an explanation of each required decision |

## Examples and explanatory standard

- Extract representative excerpts from implemented code and link each to its actual source. Explain the code line by line where ownership/control flow is non-obvious. Identify omitted code explicitly; do not present pseudocode as compiled implementation.
- Include real examples of a scalar property command, configurable selection, a continuous material edit, gizmo adoption, retained component/node removal, PutOnFloor inverse geometry and prepared-result adoption.
- Trace at least one complete Composer interaction from user intent through queue execution, capture, rendering effects, commit, undo and redo. Explain when state is captured and why.
- Explain `shared_ptr` retention specifically as used by this implementation, detached versus destroyed objects, weak destination guards, and why generation checks are necessary even when a pointer remains valid.
- Explain resource state versus cache state and why GPU representations may need rebuilding on undo. State actual restoration limits and failure guarantees.
- Explain that navigation/animation exclusions are intentional origin policy, not missing command coverage. Describe selection preference defaults and their effects on existing history and document dirty state.
- Use small diagrams/tables only where they clarify execution or ownership. Define project terms before use and cross-link chapters without circular prerequisite assumptions.
- Describe actual SDK changes required by Composer and show how an unrelated application can continue using widgets/gizmos without history.

## Completion

The guide and API references match final source signatures and behavior, all new pages are indexed, and source excerpts/relative links are reviewed. Update Composer's existing documentation with links and the new user-visible history semantics. Do not add test instructions or application verification exercises. Mark the plan complete only after documentation is delivered; record unexecuted builds/tests accurately in `progress.md`.
