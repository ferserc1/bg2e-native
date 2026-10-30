# Command/memento editing

This guide explains the implemented command and history path from an application
frame to restored scene state. It assumes C++ knowledge, but introduces Composer
scene ownership, UI callbacks and GPU-derived data before using them.

## Reading order

1. [Foundations](foundations.md) defines commands, mementos and explicit intent.
2. [App core](app-core.md) describes public types, policies and failure states.
3. [Execution and history](execution-and-history.md) follows admission, save
   identity, dispatch and replay.
4. [Continuous interactions](continuous-interactions.md) covers sliders, material
   editing and gizmos.
5. [SDK integration](sdk-integration.md) shows optional use outside Composer.
6. [Composer flow](composer-flow.md) follows one edit end to end.
7. [Scene/components](composer-scene-and-components.md), [geometry](composer-geometry.md),
   and [resources](composer-resources.md) describe retained state and limits.
8. [Origins and lifecycle](origins-and-lifecycle.md) covers exclusions and teardown.
9. [Extending](extending.md) gives the decisions for new commands and adapters.

## Vocabulary

| Term | Meaning here |
|---|---|
| Command | An explicit requested operation with execute/undo/redo behavior. |
| Memento | State retained so an operation can restore the exact prior authored value. |
| Preview | Temporary visible change during a continuous interaction. |
| Adoption | Capture an already-applied preview without applying it twice. |
| Generation | Identity of the current document lifetime; it invalidates stale asynchronous work. |
| Derived state | GPU realization, render history, selection caches or previews rebuilt from authored state. |
| Retention lease | Shared ownership keeping a detached object/resource alive while history can restore it. |

## Module and source map

Generic APIs live in `lib/include/bg2e/app/Command*.hpp` and implementations in
`lib/src/bg2e/app/`. Optional widget integration is under
`lib/include/bg2e/ui/{EditInteraction,ReflectionWidget,DrawMaterialEditor}.hpp`.
Composer adapters are in `apps/bg2e_composer/src/command/`; dispatch, document
and UI binding are in `AppDelegate`, `CommandDispatch`, `Document` and
`EditorTools` sources. The public draw API remains the production rendering
path; this work does not move rendering into `bg2e::gpu`.

## Implemented scope

Generic commands/history/queue/transactions/composites, optional SDK intent
callbacks and Composer scene, material, environment, geometry, import, UV2,
lightmap and scene-graphics-setting adapters are implemented in source. History
menus expose action names and availability. Unsupported custom/native cases
reject as described in their chapters. Source review and documentation checks
were performed; compilation and runtime behavior have not been verified by
these changes. Begin with [foundations](foundations.md).
