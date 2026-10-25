# PluginComponentsVisitor

**Header:** `<bg2e/plugin/PluginComponentsVisitor.hpp>`

**Namespace:** `bg2e::plugin`

A scene-integration NodeVisitor that matches persistent component type IDs. Most
applications should use `Plugin::removeComponents(id, root)` so the IDs come from
the plugin's actual successful registrations.

## Public API

```cpp
struct Entry {
    std::shared_ptr<scene::Node> node;
    std::shared_ptr<scene::Component> component;
    std::string type;
};

explicit PluginComponentsVisitor(std::vector<std::string> types);
void visit(scene::Node* node) override;
const std::vector<Entry>& entries() const;
size_t removeComponents();
```

Traverse a shared-owned scene with `root->accept(&visitor)`. The constructor sets
`ignoreDisabled(false)`: disabled nodes and descendants are included. Matching uses
`Component::typeName()`, not RTTI. `entries()` exposes the collected owning node/
component references. Nodes must support their normal `shared_from_this` ownership.

`removeComponents()` makes a separate pass and calls `Node::removeComponent` for
each entry still owned by its collected node. It returns the number of completed
removal calls and clears its entries before returning. Normal Node callbacks run;
callback/allocation failures can throw. Do not mutate the scene concurrently.
A fresh visitor per cleanup is simplest; collecting alone retains its matches.

The visitor does not discover other scene roots, detached objects or references in
history/UI/jobs. Do not keep a visitor (or copies of its Entries) holding plugin
components across unload. See [scene cleanup](unloading.md#remove-registered-components-from-a-scene)
and [the programmer's responsibility](unloading.md#programmer-responsibility).
