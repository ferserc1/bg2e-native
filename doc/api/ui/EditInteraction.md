# Optional edit interactions

`EditInteraction` and `EditSession` in
[`EditInteraction.hpp`](../../../lib/include/bg2e/ui/EditInteraction.hpp)
provide explicit Begin/Update/Commit/Cancel boundaries. They do not create
history. A host can connect the handler to `CommandQueue`, another transaction
system, or its own model callback.

`EditSession::beginFrame/endFrame` bracket the whole editor frame. A control not
seen for the frame cancels its preview. Call `cancel()` before changing the
target or destroying the editor. Callback payloads must own CPU values, not
borrowed object pointers. A rejected Begin must leave the model unchanged and
does not receive Cancel; handlers must not throw. Failed cancellation keeps the
session available for recovery. An asynchronous cancellation handler can return
false while restoration is queued and return true on a later retry after it
completes. `beginFrame()` retries a pending cancellation; `submit()` and
`commit()` cannot overtake it. Explicit lifecycle calls can retry too. Do not
release or rebind the session in between.

`ReflectionWidget::drawEditIntents` emits a `ReflectionPropertyPath` and typed
`std::any` proposal without invoking the live property setter. Resolve the path
again at execution and replay; returned `PropertyInfo` and owner pointers are
borrowed for that operation only. Action intents have no implicit undo
contract. Polymorphic replacement requires an adapter retaining both objects.
The legacy `drawProperties`/`drawActions` callback APIs remain available to
applications that do not opt in.

`DrawMaterialEditor::onEditRequested` is an exclusive mutation owner: in this
mode the editor does not run its own queue or changed notification. Its target
set is frozen for the interaction, and the host must complete and acknowledge
cancellation before rebind/cleanup. `DrawMaterialEditor::interactionActive()` and
`ComponentInspector::interactionActive()` expose whether their session still
owns an interaction, including pending cancellation. `onChanged` is a post-change notification
with separate CPU/resource opt-ins; it is not an intent callback. See
[`Material_Editors.md`](Material_Editors.md) and the
[SDK integration chapter](../../architecture/command-memento/sdk-integration.md).


## Live preview ownership

For viewport feedback, connect Begin to baseline capture, Update to safe model
publication, Commit to adoption of the last applied value, and Cancel to baseline
restoration. Enqueueing only at Commit leaves the viewport unchanged throughout
the gesture. Preserve the supplied interaction ID and distinguish successful
queue admission from completion. A guard must allow the owning transaction's
updates while rejecting competing interactions.

Composer uses this flow for CPU material patches and supported reflected values,
including light/environment colors. Texture/resource replacement keeps its
separate prepared publication path. This behavior is application policy; the
SDK widgets themselves do not require a command history.
