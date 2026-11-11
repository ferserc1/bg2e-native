# Component issues and reversible fixes

`<bg2e/scene/ComponentIssues.hpp>` defines diagnostics independent of reflection
and UI. Components override these queries, whose default implementations report
nothing:

```cpp
std::optional<ComponentIssue> checkCompatibility(const Component& other) const;
void collectDependencyIssues(const ComponentValidationContext& context,
    std::vector<ComponentIssue>& issues) const;
```

Queries must not mutate objects, load resources, or depend on the inspected node
being the component's current owner. `Warning` describes problematic, ambiguous
or undefined behavior; `Error` describes an unavailable essential dependency.
Neither severity changes `Node::addComponent()` or blocks serialization.

Compatibility is directional. The validator evaluates each ordered pair, and
attributes the result to the declaring component. Text reports its overlap with
another Drawable; Drawable does not know about Text. Inheritance itself never
implies incompatibility. Type requirements accept compatible derived classes.

`ComponentIssue` contains a stable code, severity, message, help and an array of
`ComponentIssueFix` alternatives. Each fix has a button label, a description of
its effects, and a read-only factory returning a fresh `unique_ptr<app::Command>`.
One command implements the entire solution, including its memento and rollback.
Independent missing dependencies produce independent issues.

## Collecting diagnostics

```cpp
auto entries = bg2e::scene::validateNodeComponents(node);
```

Each entry identifies its declaring component and optional conflicting component
using weak references. A future scene browser can traverse nodes and concatenate
these results. No global issues window or status-bar integration is provided.

An optional second argument scopes external dependencies and root fixes to a
particular document subtree. The inspected node must belong to that subtree.
Composer uses this to exclude its runtime tool nodes. Direct query callers can
supply the same scope through `ComponentValidationContext::root`; `scene` can be
null for detached nodes.

## Execution and history

`ComponentInspector::onIssueFixRequested()` transfers an unexecuted command to
the application. Composer wraps it in `FixSceneComponentCommand` and sends it
through its existing safe dispatcher/history. The wrapper preserves editor
state, validates the resulting editing graph and refreshes references/tools.

Without a handler, the inspector schedules execution through
`MainLoop::safeUpdateScene()`, reports failure and calls `onChanged` after an
applied change. It does not retain a history. Custom applications can execute the
same command at their safe scene boundary, or adapt it to another history.
Custom resource commands remain responsible for resource retention/accounting;
applications own native resource preparation, retirement and synchronization.

Built-in factories capture targets weakly. Execution rechecks the current state,
then captures exact component membership and any modified authored values. Undo
restores the captured previous state; redo uses retained forward state and the
same created component identities. Existing transformations are never removed
by a repair which did not create them. Membership changes outside history reject
replay instead of silently discarding unrelated components. Partial publication
is compensated; failed compensation returns `Faulted` under `app::Command`'s
existing contract.

## Initial rules

| Declaring component | Condition | Severity | Fix |
| --- | --- | --- | --- |
| Sun | No same-node Environment | Warning | Add Environment |
| Environment | Neither same-node Sun nor configured skybox image | Warning | Create/convert Sun and add missing Transform |
| Text | No TextFonts in the document | Error | Add bundled font to the document root |
| Text | Required font unconfigured, ambiguous or unavailable | Error | Configure bundled font when the identifier is absent |
| Text | Another Drawable on the node | Warning | Manual separation |
| Camera | No same-node Transform | Error | Add Transform |
| OrbitCamera | No same-node Transform | Error | Add Transform |
| OrbitCamera | No same-node Camera | Warning | Add perspective Camera and missing Transform |
| Input/Output ChainJoint | No Chain on the immediate parent | Warning | Manual hierarchy correction |
| PolarTransformController | No same-node Transform | Error | Add Transform |
| FixedScaleTransformController | No same-node Transform | Error | Add Transform |

The font fixes use the bundled Didact Gothic font and its notices, registered
under the text's required identifier. Existing font declarations are preserved;
a fix never silently overwrites an authored declaration. Unavailable or duplicate
configured fonts require manual correction. `Scene::textFontAvailable()` quietly
inspects the prepared registry; inspection never triggers font preparation or
renderer warning logs. An unprepared registry is not treated as a failed load.
