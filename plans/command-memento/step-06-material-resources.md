# Step 6 — Material, texture and environment commands

Dependency: step 5.

## Work

- Implement grouped Composer material/property commands, texture assignment/clear/settings, indirect mode, material paste and prepared patch application. Copying material remains a clipboard action without a document history entry.
- Replace internal material mutation paths with optional intents routed through the dispatcher for Composer. Capture before/after at safe execution time and make color/numeric sessions commit one entry.
- Preserve independent material ownership and synchronize authoritative material definitions with live draw materials. Undo must restore authored state as well as the visible realization.
- Retain old/new texture references and required generated/temporary image sources. A filesystem path alone cannot guarantee redo after an external file changes. Preserve device identity and existing retirement rules.
- Leave `TextureCache` behavior unchanged. No cache rollback, reload manager, scene-directory garbage collection or global cache refactor.
- Implement environment-property commands and prepared environment-preset adoption. Read/parse preset files outside commands; restoring the adopted environment is undoable. Preserve active environment selection, component identity and last-good GPU publication behavior.
- Prepare failing resource operations before publishing bindings where possible. Make multi-target publication recoverable; no history entry or cursor movement on failure. Escalate failed rollback to the explicit fault state.
- Distinguish history grouping from expensive-work throttling: environment generation may coalesce compatible absolute requests, but commit must apply the final accepted state and cancel must restore the initial state/resources.

## Completion

Material/resource/environment edits use command-owned restoration, optional continuous sessions and unchanged global cache semantics. Record retained-resource ownership and SDK changes for documentation. No Composer tests.
