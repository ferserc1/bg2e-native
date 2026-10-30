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
session available for recovery.

`ReflectionWidget::drawEditIntents` emits a `ReflectionPropertyPath` and typed
`std::any` proposal without invoking the live property setter. Resolve the path
again at execution and replay; returned `PropertyInfo` and owner pointers are
borrowed for that operation only. Action intents have no implicit undo
contract. Polymorphic replacement requires an adapter retaining both objects.
The legacy `drawProperties`/`drawActions` callback APIs remain available to
applications that do not opt in.

`DrawMaterialEditor::onEditRequested` is an exclusive mutation owner: in this
mode the editor does not run its own queue or changed notification. Its target
set is frozen for the interaction, and the host must synchronously settle
cancellation before rebind/cleanup. `onChanged` is a post-change notification
with separate CPU/resource opt-ins; it is not an intent callback. See
[`Material_Editors.md`](Material_Editors.md) and the
[SDK integration chapter](../../architecture/command-memento/sdk-integration.md).
