# Composer flow

## One scalar edit from intent to replay

The following excerpt is from the actual typed-reflection handler. It shows that
the widget draft does not mutate the scene and that a command is submitted only
at Commit. The surrounding validation and error handling are omitted here.

```cpp
if (phase == EditPhase::Commit)
{
    command::Effects effects;
    effects.lightmapSources = true;
    effects.renderHistories = true;
    return submitSceneCommand(
        std::make_unique<command::component::EditProperty>(_stage, component, intent), effects).accepted();
}
return !commandFaulted();
```

Source: [`AppDelegate::componentEditIntent`](../../../apps/bg2e_composer/src/SceneCommands.cpp).
`EditSession` supplies the phase and immutable `intent`; `EditProperty` resolves
the reflected path at operation time. `submitSceneCommand` routes through
`CommandDispatch` to the safe graphics boundary. The command captures the old
authored value and applies the proposal; declared effects invalidate relevant
render histories/lightmap sources. Undo resolves the target afresh and restores
the old value. Redo restores the captured new value without consulting current
selection or invoking the widget.

The important capture moment is command execution, after FIFO predecessors have
completed and immediately before publication. Continuous edits instead capture
at Begin and adopt the live final preview at Commit.

## Prepared result path

UV2 and lightmap results are computed in private CPU/resource state. Their
command is submitted with `CommandOrigin::PreparedResult`; the initial validator
checks source token, document generation, original target and job identity.
Only after complete preparation does one command publish. Completion reports
actual publication and fires once. Undo/redo rely on retained prepared values,
not the expired UI token or mutable source files.

## Effects and menus

Composer declares effects (lightmap sources, render histories, scene references,
tools, selection) at dispatch. Commands do not directly call every UI subsystem.
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
