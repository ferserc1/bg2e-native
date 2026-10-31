# Command history API

`bg2e::app` provides optional command infrastructure. It does not observe scene
setters and does not depend on Composer. Include `<bg2e/app/all.hpp>` or the
individual headers. The public declarations are in [`Command.hpp`](../../../lib/include/bg2e/app/Command.hpp),
[`CommandHistory.hpp`](../../../lib/include/bg2e/app/CommandHistory.hpp),
[`CommandQueue.hpp`](../../../lib/include/bg2e/app/CommandQueue.hpp),
[`CommandTransaction.hpp`](../../../lib/include/bg2e/app/CommandTransaction.hpp),
and [`CompositeCommand.hpp`](../../../lib/include/bg2e/app/CompositeCommand.hpp).

## Implement a command

```cpp
class SetExposure final : public bg2e::app::Command {
public:
    const bg2e::app::CommandInfo& info() const noexcept override;
    bg2e::app::CommandMemory memory() const override;
    bg2e::app::CommandResult execute() noexcept override;
    bg2e::app::CommandResult undo() noexcept override;
    bg2e::app::CommandResult redo() noexcept override;
};
```

The class must capture target identity and before/after values and must not let
exceptions escape its operations. `Rejected`, `Cancelled`, `Failed` and `NoOp`
must leave the model unchanged; partial work must be compensated. A failed
compensation returns `Faulted`. `CommandResult::message` is a `string_view` and
must refer to static-lifetime text. `CommandInfo` declares display name,
category, history policy (`Never`, `Always`, `UserPreference`), document impact
and undoability. Policy is resolved at admission, so later preference changes
do not rewrite existing entries. `CommandMemory` reports local retained bytes
and immutable resource-version tickets. Reuse one ticket identity only when all
users declare the same byte count.

## History and execution

`CommandHistory` defaults to 256 entries and 64 MiB. It is owner-thread only;
commands, callbacks and destruction use that same thread. `prepare` returns an
admission result and move-only reservation. `execute(reservation)` consumes it;
the convenience overload prepares and executes directly. `undo` and `redo`
operate on the cursor. `documentState`, `savedState` and `dirty` distinguish
document identity from history count. Call `markSaved()` only after successful
IO and with no active preview. `noteNonhistoricalChange` requires explicit
compatibility. Replacing a document requires a greater generation in
`resetBaseline`; pending reservations from older generations become stale.

History never evicts entries automatically to satisfy a limit. Admission and
growth are rejected if the reservation cannot fit. Applications can call
`discardOldest` or `discardRedo` at a stable boundary and adjust limits with
`setLimits`. `CommandReleaseSink` can defer destruction; its implementation
must retain both the command and its resource accounting until the caller's
safe retirement boundary, and must not re-enter history.

## Queue and transaction

`CommandQueue` takes a caller-owned executor. `pump()` schedules at most one
head; the executor invokes either run or cancel exactly once at an appropriate
boundary. Queue callbacks run on the history owner thread. Pending jobs are
limited to 1024 and update payloads have a configurable byte ceiling. Adjacent
unscheduled absolute updates coalesce only when interaction, generation and
exact target/property key match. Ordinary updates keep FIFO order. Always call
`shutdown()` at a safe boundary before destroying history or device resources.

Implement continuous actions as `CommandTransaction`: `begin()` captures the
baseline once, `update(any)` applies a preview and restores the previous preview
if that update fails, and `changed()` determines whether commit records an
entry. `memoryForUpdate` must bound before, redo, preview and rollback state
before each update. History exposes begin/update/commit/cancel methods; the
queue exposes corresponding enqueue methods. Transaction ID checks prevent
late updates from crossing interactions. Commit adopts the already-applied final
state and never calls `execute()` a second time. `changed()` must compare final
values with the baseline, so an interaction that returns to its initial values
creates no entry. `dirty()` includes an effective document preview even before
Commit. Cancel invokes the transaction's `undo()`; failed restoration retains
ownership for recovery.

Composer connects CPU material values and reflected component/environment values
to these transactions through renderer-boundary callbacks. Its deferred Commit
acknowledges admission; Cancel acknowledges completed restoration. This host
transport is separate from the generic `CommandQueue` transaction API; see
[continuous interactions](../../architecture/command-memento/continuous-interactions.md).

`CompositeCommand` is one history entry. Its immutable children must be
undoable, share category and policy, and have compatible document impact. It
compensates children completed before a failure; failed compensation faults the
history. See the [architecture guide](../../architecture/command-memento/index.md)
for execution states, examples and application integration.

## Applications without history

No SDK widget or gizmo requires these classes. An application may continue to
mutate in its existing callback, use `EditSession` only to group its own
callbacks, or connect typed intents to its own undo system. Notifications such
as `onChanged` remain notifications after mutation; they are not safe command
requests. A supervised edit must receive immutable proposed values before
mutation and own validation, scheduling, capture and rollback.
