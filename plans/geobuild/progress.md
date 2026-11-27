# Geometry builder progress

Read the [execution protocol](summary.md#execution-protocol) before acting. Current is the only implementation unit authorized by a request to continue.

| Field | Value |
|---|---|
| State | READY |
| Current | [4.3 Construct closed primitives with source labels](step_4/substep_3.md) |
| Next after validation | [4.4 Evaluate the ordered boolean sequence](step_4/substep_4.md) |
| Successor preview | Evaluate the ordered primitive operations through Manifold. |
| Current summary | Step 4.2 accepted. Step 4.3 has not started. |
| Verification evidence | No implementation checks have run for 4.3. |
| User validation | None pending. |
| Blocker | None. |

## Validation log

Append one line per accepted substep with its identifier, user validation and any explicitly deferred checks.

- **1.1 accepted by the user.** git diff --check passed. A source-level Python check verified all 32 bit positions, 0x80000000 for slot 31, 0x80000001 for the union of slots 0 and 31, and 0xffffffff for all slots. Source review confirmed that activation rejects zero, combined and unregistered slots without changing state, and that reactivation succeeds without a state write. C++ compilation, runtime behavior and application compatibility remain unverified. No Composer run was reported.

- **1.2 accepted by the user.** Composer validation confirmed by the user. git diff --check passed. Source trace for Scene, Volume and a third registered slot: scene-only Open reaches all three surfaces only in Scene, shared Save reaches all three in every slot, a toolbar-only action can appear in layout and customization without a menu entry, and a Scene-or-Volume action reaches all three surfaces in those two slots only. Layout placement remains explicit and the picker lists eligible actions absent from the active layout. Source review also checked malformed or duplicate batch rejection before publication and the existing Composer registration path. Compilation was not run by the assistant.

- **1.3 accepted by the user.** Composer validation confirmed by the user. git diff --check passed. Source review traced platform modifier normalization, disjoint Delete handlers, shared Save, an action switching the active slot, disabled and text-input guards, legacy registrations and collection teardown. Added tests/app/workspace_shortcuts/main.cpp for key-up, overlap rejection and batch rollback, exactly-once dispatch, disabled actions, rebinding and teardown/recreation. The fixture was not compiled or executed. Focused text input was not exercised by the assistant.

- **1.4 accepted by the user.** Composer validation confirmed by the user. git diff --check passed. Source review traced explicit empty sides, hidden actions, separators, obsolete and duplicate ID cleanup against the projected catalog, bad-file fallback, and the legacy single-toolbar load path. Added tests/ui/workspace_toolbar/main.cpp for distinct preference files, path alias rejection, first-time write, legacy load and file preservation across activation. The fixture was not compiled or run. The two-workspace in-memory layout round trip remains unverified.

- **2.1 accepted by the user.** Composer validation confirmed by the user. git diff --check passed. Source comparisons verified action definitions, titles, settings filename, relative includes, and unchanged setup, resize, draw order and callback bodies apart from the new receiver and dimensions. Weak callbacks target the SceneWorkspace owned by EditorUi. Activation only selects the layout. Compilation was not run by the assistant.

- **2.2 accepted by the user.** Composer validation confirmed by the user. git diff --check passed. Source comparison verified 41 unique existing IDs with unchanged menu paths, exactly four shared actions, and nine unchanged shortcut definitions. Reviewed predicates, settings ownership, preferences paths and the hardwired Escape path. Compilation was not run by the assistant. Cross-workspace interaction remains unverified.

- **2.3 accepted by the user.** Composer validation confirmed by the user. git diff --check passed. Reviewed readiness, pending-work and same-slot paths, cancellation and the adjacent eviction calls. Added tests/app/workspace_transition/main.cpp covering clean and dirty documents with undo and redo, refusal during a reserved edit, state preservation and fresh history. The fixture was not compiled or run. Effective workspace switches remain unverified.

- **3.1 accepted by the user.** Composer validation confirmed by the user. git diff --check passed. An independent Python calculation checked 0.173 relative, 0.15 absolute, signed half steps and reversals. Added tests/manipulation/linear_snapping/main.cpp for these cases, continuous motion, alignment gating, invalid steps and gesture snapshots. The C++ fixture was not built or run, and preference persistence at runtime remains unverified.

- **3.2 accepted by the user.** Composer validation confirmed after the absolute alignment correction. git diff --check passed. An independent numeric check verified all three world axes and a tilted axis under rotated, nonuniformly scaled parents, including signed displacement, reversals and stationary samples. Source review checked the shared gesture calculation, matching world frames for draw, pick and drag and existing commit/cancel paths. Compilation and live checks were not run by the assistant.

- **4.1 accepted by the user.** Composer smoke check accepted. git diff --check passed. Inspected header dependencies and seed changes for add, duplicate, remove and both move directions. Added tests/geobuild/recipe/main.cpp covering order, ID stability, seed roles, scaled/reflected/sheared transforms and rejection of invalid transforms. The C++ fixture was not compiled or run by the assistant.

- **4.2 accepted by the user.** Composer validation confirmed. git diff --check passed. Verified the release commit, archive checksum, byte-for-byte source copies and parity with the 18 upstream core source entries. Reviewed private linkage and Distribution/standalone install rules. CMake configure/build, packaging and standalone consumer checks were not run by the assistant on Linux, macOS or Windows.
