# Composer flow

## One scalar edit from intent to replay

A reflected property edit enters `AppDelegate::componentEditIntent` with the
widget's interaction ID and immutable `ReflectionEditIntent`. Supported values
are passed to `valueEditIntent`, which schedules each phase at a renderer
boundary. The UI widget does not call a live setter.

1. Begin reserves interaction dispatch and creates `EditProperty`; its
   `begin()` captures the original authored value and stage state.
2. Each Update resolves the frozen reflected path, validates and applies the
   proposal, updates the final snapshot, invalidates derived histories and
   requests a frame. The viewport therefore changes while the control is active.
3. Commit follows all preceding updates and calls `commitTransaction()`. History
   adopts the already-applied result as one entry without executing it again.
4. Cancel restores the baseline. A return to the original values creates no
   history entry. Undo and redo restore the captured initial/final values.

Sources: [`AppDelegate::componentEditIntent`](../../../apps/bg2e_composer/src/SceneCommands.cpp),
[`valueEditIntent`](../../../apps/bg2e_composer/src/CommandDispatch.cpp), and
[`EditProperty`](../../../apps/bg2e_composer/src/command/component/ComponentCommands.cpp).
Material CPU values use the same scheduling with `materialInteraction()`;
resource edits remain privately prepared discrete commands. See
[continuous interactions](continuous-interactions.md) for cancellation
acknowledgement and failure recovery.

## Prepared result path

UV2 and lightmap results are computed in private CPU/resource state. Their
command is submitted with `CommandOrigin::PreparedResult`; the initial validator
checks source token, document generation, original target and job identity.
Only after complete preparation does one command publish. Completion reports
actual publication and fires once. Undo/redo rely on retained prepared values,
not the expired UI token or mutable source files.

## Effects and menus

Composer declares effects (lightmap sources, render histories, scene references,
tools, selection) at dispatch. Live value transactions receive an application
effects callback used for preview and replay.
Menu labels use `CommandHistory::undoName/redoName`; availability includes busy
IO/import/publication/queue and fault state. Cmd-Z on macOS, Ctrl-Z elsewhere;
Shift adds redo. Text input owns its own undo handling. Selection recording is
an opt-in preference (`draw.bg2e_composer.history`, `recordSelection`), default
off, saved explicitly. It affects future admissions only and selection entries
do not dirty the document.

## Exact modules

`Document::commands` owns the per-document `Dispatcher`; its queue delegates
safe scheduling to Composer. `CommandDispatch` coordinates interaction
settlement, draft publication and deferred reads. `SceneEditor` and `EditorTools`
bind typed property/material intents. `ImportActions` and `EditorBaking` hand
prepared candidates into scene/material commands. See
[scene](composer-scene-and-components.md) and
[resources](composer-resources.md).
