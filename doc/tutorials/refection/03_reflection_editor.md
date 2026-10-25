# Part 3 — Build a node editor from reflection

[Previous: Create a rotating component](02_rotation_component.md) ·
[Start: Scene and manipulation](01_scene_and_manipulation.md)

The built-in `NodeEditor` delegates component inspection to engine widgets. We
will now build a second window with the same essential workflow: edit the node's
name/enabled state, list its components, add/remove components, edit properties
and invoke actions. The new editor discovers types and builds controls from
metadata; it never checks for `RotationComponent` by C++ type.

We will implement float and enum widgets ourselves, because they are the two
property types that drive rotation. For the other scalar types we will use
`ReflectionWidget::editValue()`. For nested objects and polymorphic ownership we
will use `ReflectionWidget::drawProperties()` on one property at a time. This
keeps the tutorial focused on reflection consumers while retaining the engine's
recursive-path and resource-picker behavior. These are public `bg2e::ui` wrappers;
none of the application files includes or calls ImGui.

Finally, an optional Reflection Samples component will make every property kind
available for hands-on inspection, including object nesting, subtype replacement
and resources that are ordinary sample paths rather than renderer assets.

## 1. Define the editor's public entry point

Create `examples/tutorials/reflection/src/TutorialNodeEditor.hpp`. Assemble the
following blocks in their displayed order. The class and namespace stay open
until Step 5. Start with the GPL header, includes and public method:

```cpp
/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <bg2e/reflection/Registry.hpp>
#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Group.hpp>
#include <bg2e/ui/Numeric.hpp>
#include <bg2e/ui/ReflectionWidget.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Value.hpp>

#include <algorithm>
#include <any>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace tutorial {

class TutorialNodeEditor {
  public:
    using NodeOperation = std::function<void(bg2e::scene::Node&)>;
    using Queue = std::function<void(NodeOperation)>;

    void draw(bg2e::scene::Node* node, const Queue& queue)
    {
        using namespace bg2e;
        if (!node)
        {
            ui::Text::text("No selection");
            return;
        }
        auto name = node->name();
        auto enabled = node->enabled();
        if (ui::Value::text("Node name", name))
        {
            queue([name](scene::Node& target)
            {
                target.setName(name);
            });
        }
        if (ui::Button::checkBox("Enabled", &enabled))
        {
            queue([enabled](scene::Node& target)
            {
                target.setEnabled(enabled);
            });
        }
        drawAddComponent(*node, queue);
        if (ui::Group::beginTree("Reflection registry"))
        {
            auto& registry = reflection::TypeRegistry::get();
            ui::Text::text("Registered types: " + std::to_string(registry.typeNames().size()));
            for (const auto& component : node->orderedComponents())
            {
                const auto key = component->typeName();
                ui::Text::text(key + (registry.contains(key) ? " (registered)" : " (not registered)") +
                    "; object depth: " + std::to_string(registry.objectChainDepth(key)));
            }
            ui::Group::endTree();
        }

        int id = 0;
        for (const auto& component : node->orderedComponents())
        {
            ui::Group::pushId(id++);
            const auto* info = reflection::TypeRegistry::get().type(component->typeName());
            if (!info)
            {
                ui::Text::text(component->typeName() + ": no reflection metadata");
                ui::Group::popId();
                continue;
            }
            const auto title = info->displayName.empty() ? info->typeName : info->displayName;
            if (ui::Group::collapsingHeader(title))
            {
                if (ui::Button::button("Remove"))
                {
                    queue([component](scene::Node& target)
                    {
                        if (component->ownerNode() == &target)
                        {
                            target.removeComponent(component);
                        }
                    });
                }
                // Retain the component, but recheck membership when the edit executes.
                const ui::ReflectionWidget::EditHandler edit = [queue, component](auto operation, bool)
                {
                    queue([component, operation = std::move(operation)](scene::Node& target)
                    {
                        if (component->ownerNode() == &target)
                        {
                            operation(component.get());
                        }
                    });
                };
                drawProperties(component.get(), *info, edit);
                drawActions(*info, edit);
            }
            ui::Group::popId();
        }
    }
```

`Queue` is an application-supplied scheduling function. It accepts a
`NodeOperation`, which receives the live node only when the queued edit executes.
The editor does not own the render delegate and does not decide when GPU work
may safely be interrupted. This separates interface construction from mutation
scheduling without hiding how either works.

The first controls use temporary copies of the node name and enabled flag. When
a widget reports a change, we capture the new value by value in an operation.
The live node is not modified while those controls are drawn.

`orderedComponents()` provides the components in their execution order. For
each component, `type(component->typeName())` retrieves the description. An
unregistered component still gets an explanatory label, but has no generic
property editor. There is no fallback that guesses its data layout.

`Group::pushId()`/`popId()` scope repeated labels so multiple components can have
controls called “Remove” or “active”. The `##` suffixes below give a widget a
stable internal ID while leaving its visible label readable. Match every push
with a pop and every successful `beginTree()` with an `endTree()`.

The collapsible registry section demonstrates discovery independently of the
component factory: `typeNames()` lists registered reflected types, `contains()`
checks membership, and `objectChainDepth()` reports ordinary Object nesting.
These are metadata queries, not a way of finding scene instances.

For property edits we capture a `shared_ptr` to the component and check its
`ownerNode()` again when the operation executes. Retaining it prevents destruction;
the owner check prevents editing a retained component that has been removed.
The boolean accepted by `ReflectionWidget::EditHandler` indicates whether an
operation may replace resources. Our tutorial always chooses the conservative
queue, so it does not need to optimize based on this hint.

## 2. Build widgets for values, not for component classes

Append the private section and the scalar editor:

```cpp
  private:
    static bool editScalar(std::any& value, const bg2e::reflection::PropertyInfo& property)
    {
        using namespace bg2e;
        const auto label = (property.metadata.displayName.empty()
            ? property.name : property.metadata.displayName) + "##" + property.name;
        // The two types used by RotationComponent are implemented explicitly.
        // Other scalar types use the engine's existing value-only widget adapter.
        if (property.type != reflection::PropertyType::Float &&
            property.type != reflection::PropertyType::Enum)
        {
            return ui::ReflectionWidget::editValue(value, property);
        }
        const bool writable = !property.isReadOnly();
        ui::Group::beginDisabled(!writable);
        bool changed = false;
        if (property.type == reflection::PropertyType::Float)
        {
            auto next = std::any_cast<float>(value);
            const auto& metadata = property.metadata;
            if (property.editor == reflection::PropertyEditor::Slider && metadata.min && metadata.max)
            {
                changed = ui::Numeric::sliderFloat(label, &next, float(*metadata.min), float(*metadata.max));
            }
            else if (property.editor == reflection::PropertyEditor::Input)
            {
                changed = ui::Numeric::number(label, &next);
            }
            else
            {
                changed = ui::Numeric::drag(label, &next, float(metadata.step.value_or(0.1)));
            }
            if (changed)
            {
                if (metadata.min)
                {
                    next = std::max(next, float(*metadata.min));
                }
                if (metadata.max)
                {
                    next = std::min(next, float(*metadata.max));
                }
                value = next;
            }
        }
        else
        {
            const auto current = std::any_cast<int64_t>(value);
            std::vector<std::string> labels;
            std::vector<int64_t> values;
            uint32_t selected = 0;
            bool found = false;
            for (const auto& [labelText, enumValue] : property.metadata.enumOptions)
            {
                if (current == enumValue)
                {
                    selected = uint32_t(labels.size());
                    found = true;
                }
                labels.push_back(labelText);
                values.push_back(enumValue);
            }
            if (!found)
            {
                selected = uint32_t(labels.size());
                labels.push_back("Unknown (" + std::to_string(current) + ")");
                values.push_back(current);
            }
            if (property.metadata.enumOptions.empty())
            {
                ui::Text::text(label + ": no enum options");
            }
            else if (ui::Value::comboBox(label, labels, selected))
            {
                value = values[selected];
                changed = true;
            }
        }
        ui::Text::tooltip(property.metadata.tooltip);
        ui::Group::endDisabled();
        return writable && changed;
    }
```

The getter result is a temporary `std::any`. Editing this temporary does not call
the real component setter. We dispatch using `PropertyInfo::type`, rather than
casting the component to a concrete behavior class.

For Float we use `any_cast<float>`, choose a slider/input/drag from
`PropertyEditor`, and read the minimum, maximum and step from metadata. A single
minimum or maximum is also respected. Our consumer treats Default, Drag and
Angle as drag controls; the shipped widget may render Angle differently. Both
consumers still edit the same numeric value in the units declared by the type.

For Enum we build separate label and value arrays. The selected combo-box index
indexes the value array; it is never sent directly to the setter. This works even
for enums with values such as 10, 20 and 100. An unknown current value gets an
explicit “Unknown” entry. An enum without options has no editable combo.

`isReadOnly()` is based on the available write path, not an independent flag.
The disabled scope communicates that state; returning `writable && changed`
ensures a caller never queues a setter for a read-only property.

`ReflectionWidget::editValue(value, property)` handles the remaining scalar cases
without invoking the original property's accessors. Its `std::any` contracts are:

| `PropertyType` | Stored value in `std::any` |
| --- | --- |
| Bool | `bool` |
| Int | `int32_t` |
| UInt | `uint32_t` |
| Float | `float` |
| Double | `double` |
| String | `std::string` |
| Vec2, Vec3, Vec4 | `glm::vec2`, `glm::vec3`, `glm::vec4` |
| Mat4 | `glm::mat4` |
| Color | `base::Color` |
| Enum | `int64_t` |
| Path | `std::filesystem::path` |
| Resource | The String or Path type preserved in `metadata.resourceValueType` |

Object and PolymorphicObject do not use scalar `std::any` getters. They expose
object accessors and are handled separately in the next step.

## 3. Enumerate properties and queue their setters

Append this method:

```cpp
    static void drawProperties(void* instance, const bg2e::reflection::TypeInfo& info,
        const bg2e::ui::ReflectionWidget::EditHandler& edit)
    {
        using namespace bg2e;
        std::vector<std::string> categories;
        for (const auto& property : info.properties)
        {
            if (std::find(categories.begin(), categories.end(), property.metadata.category) == categories.end())
            {
                categories.push_back(property.metadata.category);
            }
        }
        int id = 0;
        for (const auto& category : categories)
        {
            if (!category.empty())
            {
                ui::Text::separator(category);
            }
            for (const auto& property : info.properties)
            {
                if (property.metadata.category != category)
                {
                    continue;
                }
                ui::Group::pushId(id++);
                if (property.type == reflection::PropertyType::Object ||
                    property.type == reflection::PropertyType::PolymorphicObject)
                {
                    // This adapter handles recursive paths and subtype replacement.
                    // It resolves child pointers again when the queued edit executes.
                    reflection::TypeInfo nested;
                    nested.typeName = info.typeName;
                    nested.properties.push_back(property);
                    nested.properties.front().metadata.category.clear();
                    ui::ReflectionWidget::drawProperties(instance, nested, 0, edit);
                }
                else if (property.getter)
                {
                    auto value = property.getter(instance);
                    if (editScalar(value, property))
                    {
                        edit([setter = property.setter, value = std::move(value)](void* target)
                        {
                            setter(target, value);
                        }, property.type == reflection::PropertyType::Resource ||
                           property.type == reflection::PropertyType::Path);
                    }
                }
                ui::Group::popId();
            }
        }
    }
```

The first pass discovers categories in first-appearance order; the second draws
their properties in declaration order. You can change this presentation policy
without changing the component metadata. Display names are for humans; property
names and type keys remain the lookup identities.

For a scalar, the essential pipeline is:

```text
PropertyInfo.getter(instance)
    → a temporary std::any
    → widget chosen using type/editor/metadata
    → queued copy of setter and edited value
    → setter(live instance, value) at the update boundary
```

We copy the setter function into the queued callback. Do not retain a pointer to
a builder or a borrowed `PropertyInfo` as your delayed edit target. Register types
at startup, before inspection; replacing registry metadata while a consumer is
iterating it would require an additional lifetime policy.

For an Object or PolymorphicObject property, a one-property `TypeInfo` allows us
to reuse the recursive public adapter without asking it to draw the entire
component. We clear that copy's category because our outer loop has already drawn
the category label. `depth = 0` describes the component root; the adapter increments
it as it descends into child objects and stops at `maxObjectDepth`.

The adapter is still using reflection, not a component-specific editor. Its
algorithm is worth understanding before using it in a larger tool:

1. For Object, look up `objectTypeName`, obtain the const object pointer with
   `objectGetter`, and use `objectMutableGetter` only if a mutable path exists.
   Recursively draw the child's registered properties and actions. A missing
   mutable getter makes the entire child read-only.
2. For PolymorphicObject, use `polymorphicBaseTypeName` to find the hierarchy.
   Query `polymorphicTypeKey` to identify the active subtype. List either the
   property's `polymorphicSubtypeKeys` allow-list or all registry subtypes.
3. A type-selection change calls `polymorphicObjectReplacer` on the owner. That
   function creates the requested registered subtype and installs its shared
   owner through the builder-supplied replacement function. Replacing a subtype
   invalidates old child pointers.
4. To inspect an existing subtype, use `TypeRegistry::subtype()` to obtain its
   reflected type name, and `subtypeObject()` to cast the hierarchy's base pointer
   to the correct derived address. Ordinary `void*` casting cannot substitute
   for this adjustment, particularly with multiple inheritance. Draw both the
   hierarchy's base metadata and the concrete subtype metadata.
5. When a child setter is queued, retain the **access path**, not the borrowed
   child address. Resolve mutable child pointers again at execution time. For
   polymorphic paths, also verify the subtype key still matches the key that was
   displayed. A later replacement must not redirect an old edit into the wrong
   subtype.

You can read the complete implementation in
[`ReflectionWidget.cpp`](../../../lib/src/bg2e/ui/ReflectionWidget.cpp).
`ReflectionWidget::drawActions()` is also available when you want the standard
action layout. We implement our action loop next so the invoker API remains
visible in the tutorial.

## 4. Draw action buttons

Append:

```cpp
    static void drawActions(const bg2e::reflection::TypeInfo& info,
        const bg2e::ui::ReflectionWidget::EditHandler& edit)
    {
        using namespace bg2e;
        for (const auto& action : info.actions)
        {
            if (!action.category.empty())
            {
                ui::Text::text(action.category);
            }
            const auto label = (action.displayName.empty() ? action.name : action.displayName)
                + "##action_" + action.name;
            if (ui::Button::button(label))
            {
                edit(action.invoke, true);
            }
            ui::Text::tooltip(action.tooltip);
        }
    }
```

An action has no editable value and takes no arguments. Its `invoke` function
receives the live instance pointer. The button queues that function through the
same checked path used by property setters. This is how Rotation's Toggle rotation button
works in the custom editor. Labels, category and tooltip come from `ActionInfo`.

The UI does not know whether an action modifies a float, resets a matrix or
replaces a resource. Treat arbitrary actions conservatively unless a stronger
contract has explicitly been established by the application.

## 5. Discover constructors and finish the class

Append the following block, including the private field and closing braces:

```cpp
    void drawAddComponent(bg2e::scene::Node& node, const Queue& queue)
    {
        using namespace bg2e;
        auto& registry = reflection::TypeRegistry::get();
        auto& factories = scene::ComponentFactoryRegistry::get();
        std::vector<std::string> keys;
        std::vector<std::string> labels;
        auto typeNames = registry.typeNames();
        std::sort(typeNames.begin(), typeNames.end());
        for (const auto& key : typeNames)
        {
            const auto& components = node.orderedComponents();
            const bool exists = std::any_of(components.begin(), components.end(), [&](const auto& component)
            {
                return component->typeName() == key;
            });
            if (!factories.contains(key) || exists)
            {
                continue;
            }
            const auto* info = registry.type(key);
            keys.push_back(key);
            labels.push_back(info->displayName.empty() ? key : info->displayName);
        }
        if (keys.empty())
        {
            ui::Text::text("No components available");
            return;
        }
        if (_selectedComponent >= keys.size())
        {
            _selectedComponent = 0;
        }
        ui::Value::comboBox("Component type", labels, _selectedComponent);
        if (ui::Button::button("Add Component"))
        {
            queue([key = keys[_selectedComponent]](scene::Node& target)
            {
                // Another window may have added the component since the click.
                for (const auto& component : target.orderedComponents())
                {
                    if (component->typeName() == key)
                    {
                        return;
                    }
                }
                if (auto* component = scene::ComponentFactoryRegistry::get().createDefault(key))
                {
                    target.addComponent(component);
                }
            });
        }
    }

    uint32_t _selectedComponent = 0;
};

}
```

The Add Component list is the intersection of registered reflection types and
registered component factories, excluding types already present on the node.
This is why the two registrations in Part 2 were both necessary.
`TypeRegistry::typeNames()` does not guarantee ordering, so we sort its returned
keys explicitly to keep the menu stable across frames. The built-in inspector sorts its visible options
by display name, so the two menus need not have identical ordering.

`createDefault(key)` returns a raw component pointer that the node adopts with
`addComponent()`. It can return null for an unknown key. The queued operation
checks again for duplicates because the built-in window may have added the same
component between the click and execution.

Removal is also queued, rather than performed inside the `orderedComponents()`
loop. Removing while iterating could invalidate the traversal. The component
capture keeps it alive until the operation can check membership and remove it.

The resulting header is complete. It has no dependency on the rotation class,
no private engine headers and no ImGui calls.

## 6. Add the second window to the application

Add this include to `main.cpp`:

```cpp
#include "TutorialNodeEditor.hpp"
```

Add these private fields to `UiDelegate`, alongside the fields from Part 1:

```cpp
ui::Window _reflectionWindow;
tutorial::TutorialNodeEditor _tutorialEditor;
```

In its constructor, after configuring `_nodeWindow`, add:

```cpp
_reflectionWindow.setTitle("Node editor (reflection tutorial)");
_reflectionWindow.setPosition(1000, 10);
_reflectionWindow.setSize(370, 770);
```

Replace Part 1's `drawNodeWindows()` method with this complete version:

```cpp
    void drawNodeWindows(const std::shared_ptr<SceneDelegate>& delegate)
    {
        auto node = firstSelectedNode(*delegate->selection());
        // Keep the selected node alive throughout both immediate-mode draw calls.
        _nodeEditor.setNode(node.get());
        _nodeWindow.draw([&]
        {
            ui::Text::text("Editing the first selected node, including during multi-selection.");
            _nodeEditor.draw();
        });
        _reflectionWindow.draw([&]
        {
            const auto weakDelegate = _delegate;
            const auto weakNode = std::weak_ptr<scene::Node>(node);
            const tutorial::TutorialNodeEditor::Queue queue = [weakDelegate, weakNode](auto operation)
            {
                if (auto active = weakDelegate.lock())
                {
                    active->enqueueRendererUpdate(active->rendererGeneration(),
                        [weakDelegate, weakNode, operation = std::move(operation)](draw::Renderer& renderer)
                    {
                        auto owner = weakDelegate.lock();
                        auto target = weakNode.lock();
                        if (!owner || !target || !owner->scene() ||
                            target->sceneRoot() != owner->scene().get())
                        {
                            return;
                        }
                        renderer.invalidateHistories();
                        operation(*target);
                        owner->selection()->refreshSelection();
                        owner->state().sceneContext->updateAll();
                    });
                }
            };
            _tutorialEditor.draw(node.get(), queue);
        });
    }
```

Both windows receive the same retained `node`. The second editor does not have
its own selection manager, so there is no opportunity for the windows to choose
different objects. Keep Part 1's chronological selection callback unchanged.

The `Queue` captures weak references to the render delegate and selected node.
When enqueuing, `rendererGeneration()` binds the callback to the current renderer.
When executing, we lock both objects and verify that the node's scene root is
still the delegate's current scene root. A node can remain alive after removal
because another object retains it; a successful weak lock alone is not a scene
membership check.

`enqueueRendererUpdate()` executes arbitrary changes at the draw delegate's
pre-acquisition update boundary under its conservative resource-update scope.
It is appropriate for this tutorial's generic setters, actions and component
changes. Use the callback's renderer reference, rather than capturing an old
renderer pointer. `invalidateHistories()` prevents temporal accumulation from
reusing images from before an explicit edit. `refreshSelection()` prunes removed
geometry/component selections, and `sceneContext->updateAll()` refreshes the
scene's derived collections. Successful queued updates request a scene frame;
continuous rendering also keeps rotation running.

These checks protect delayed edits. They do not turn direct `NodeEditor` setters
into queued operations. The first window intentionally demonstrates the existing
engine widget as shipped. The second demonstrates the explicit scheduling policy
for application-owned inspection. Do not call the whole immediate-mode UI from
inside a deferred callback: it must be drawn during the UI frame.

## 7. Add a complete property playground

Rotation exercises float, enum and member-function action registration. To try
the remaining reflection families without changing the engine or using renderer
resources as experimental data, add an optional component containing sample
values. It does not affect graphics or animation.

Create `examples/tutorials/reflection/src/ReflectionSamplesComponent.hpp` with
this complete content:

```cpp
/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/reflection/Registration.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace tutorial {

struct SampleValues {
    bool active = true;
    int32_t count = 2;
    uint32_t mask = 7;
    float weight = 0.5f;
    double precision = 0.125;
    std::string title = "Reflection playground";
    glm::vec2 uv = glm::vec2(1.0f);
    glm::vec3 offset = glm::vec3(0.0f);
    glm::vec4 vector = glm::vec4(1.0f);
    glm::mat4 matrix = glm::mat4(1.0f);
    bg2e::base::Color tint = bg2e::base::Color::White();
    std::filesystem::path path;
    std::filesystem::path image;
    std::string imageName;
};

struct SampleMode {
    virtual ~SampleMode() = default;
    bool active = true;
};

struct SmoothMode final : SampleMode {
    float smoothing = 0.5f;
};

struct StepMode final : SampleMode {
    uint32_t steps = 4;
};

class ReflectionSamplesComponent final : public bg2e::scene::Component {
  public:
    BG2E_COMPONENT_TYPE_NAME("tutorial.ReflectionSamples");

    ReflectionSamplesComponent()
    {
        setIgnoreSerialization(true);
    }

    const SampleValues& values() const
    {
        return _values;
    }
    SampleValues& editableValues()
    {
        return _values;
    }
    const SampleMode* mode() const
    {
        return _mode.get();
    }
    SampleMode* editableMode()
    {
        return _mode.get();
    }
    void setMode(std::shared_ptr<SampleMode> mode)
    {
        _mode = std::move(mode);
    }
    std::string description() const
    {
        return "Sample values only; these do not affect rendering";
    }

    void reset()
    {
        _values = SampleValues{};
    }

    std::shared_ptr<bg2e::scene::Component> clone() const override
    {
        auto copy = std::make_shared<ReflectionSamplesComponent>();
        copy->_values = _values;
        copy->setPriority(priority());
        copy->setIgnoreSerialization(ignoreSerialization());
        if (const auto* smooth = dynamic_cast<const SmoothMode*>(_mode.get()))
        {
            copy->_mode = std::make_shared<SmoothMode>(*smooth);
        }
        else if (const auto* step = dynamic_cast<const StepMode*>(_mode.get()))
        {
            copy->_mode = std::make_shared<StepMode>(*step);
        }
        else
        {
            copy->_mode.reset();
        }
        return copy;
    }

  private:
    SampleValues _values;
    std::shared_ptr<SampleMode> _mode = std::make_shared<SmoothMode>();
};

inline void registerReflectionSamples()
{
    using namespace bg2e;
    static const bool registered = []
    {
        auto& registry = reflection::TypeRegistry::get();
        reflection::TypeInfoBuilder<SampleValues> values("tutorial.SampleValues");
        values.displayName("Sample values");
        values.accessor<bool>("active", [](const SampleValues& object)
        {
            return object.active;
        }, [](SampleValues& object, const bool& value)
        {
            object.active = value;
        }).category("Values").checkbox();
        values.accessor<int32_t>("count", [](const SampleValues& object)
        {
            return object.count;
        }, [](SampleValues& object, const int32_t& value)
        {
            object.count = value;
        }).category("Values").input().range(-10, 10);
        values.accessor<uint32_t>("mask", [](const SampleValues& object)
        {
            return object.mask;
        }, [](SampleValues& object, const uint32_t& value)
        {
            object.mask = value;
        }).category("Values").drag().step(1);
        values.accessor<float>("weight", [](const SampleValues& object)
        {
            return object.weight;
        }, [](SampleValues& object, const float& value)
        {
            object.weight = value;
        }).category("Values").slider().range(0, 1);
        values.accessor<double>("precision", [](const SampleValues& object)
        {
            return object.precision;
        }, [](SampleValues& object, const double& value)
        {
            object.precision = value;
        }).category("Values").input();
        values.accessor<std::string>("title", [](const SampleValues& object)
        {
            return object.title;
        }, [](SampleValues& object, const std::string& value)
        {
            object.title = value;
        }).category("Values").input();
        values.accessor<glm::vec2>("uv", [](const SampleValues& object)
        {
            return object.uv;
        }, [](SampleValues& object, const glm::vec2& value)
        {
            object.uv = value;
        }).category("Values").drag().step(0.01);
        values.accessor<glm::vec3>("offset", [](const SampleValues& object)
        {
            return object.offset;
        }, [](SampleValues& object, const glm::vec3& value)
        {
            object.offset = value;
        }).category("Values").drag();
        values.accessor<glm::vec4>("vector", [](const SampleValues& object)
        {
            return object.vector;
        }, [](SampleValues& object, const glm::vec4& value)
        {
            object.vector = value;
        }).category("Values").input();
        values.accessor<glm::mat4>("matrix", [](const SampleValues& object)
        {
            return object.matrix;
        }, [](SampleValues& object, const glm::mat4& value)
        {
            object.matrix = value;
        }).category("Values");
        values.accessor<base::Color>("tint", [](const SampleValues& object)
        {
            return object.tint;
        }, [](SampleValues& object, const base::Color& value)
        {
            object.tint = value;
        }).category("Values").colorEditor();
        values.accessor<std::filesystem::path>("path", [](const SampleValues& object)
        {
            return object.path;
        }, [](SampleValues& object, const std::filesystem::path& value)
        {
            object.path = value;
        }).category("Values");
        values.accessor<std::filesystem::path>("image", [](const SampleValues& object)
        {
            return object.image;
        }, [](SampleValues& object, const std::filesystem::path& value)
        {
            object.image = value;
        }).category("Values").resource("Images", {"png", "jpg"}).clearableResource().resourceEmptySelection();
        values.accessor<std::string>("imageName", [](const SampleValues& object)
        {
            return object.imageName;
        }, [](SampleValues& object, const std::string& value)
        {
            object.imageName = value;
        }).category("Values").resource("Images", {"png", "jpg"}, true).clearableResource();
        registry.registerType(values.build());

        reflection::TypeInfoBuilder<SampleMode> base("tutorial.SampleMode");
        base.accessor<bool>("active", [](const SampleMode& mode)
        {
            return mode.active;
        }, [](SampleMode& mode, bool value)
        {
            mode.active = value;
        }).checkbox();
        registry.registerType(base.build());
        reflection::TypeInfoBuilder<SmoothMode> smooth("tutorial.SmoothMode");
        smooth.accessor<float>("smoothing", [](const SmoothMode& mode)
        {
            return mode.smoothing;
        }, [](SmoothMode& mode, float value)
        {
            mode.smoothing = value;
        }).slider().range(0, 1);
        registry.registerType(smooth.build());
        reflection::TypeInfoBuilder<StepMode> step("tutorial.StepMode");
        step.accessor<uint32_t>("steps", [](const StepMode& mode)
        {
            return mode.steps;
        }, [](StepMode& mode, uint32_t value)
        {
            mode.steps = value;
        }).drag().step(1);
        registry.registerType(step.build());
        registry.registerSubtype<SampleMode, SmoothMode>(
            "tutorial.SampleMode", "smooth", "Smooth", "tutorial.SmoothMode");
        registry.registerSubtype<SampleMode, StepMode>(
            "tutorial.SampleMode", "step", "Step", []
            {
                return std::make_shared<StepMode>();
            }, "tutorial.StepMode");

        using Samples = ReflectionSamplesComponent;
        reflection::TypeInfoBuilder<Samples> component(Samples::staticTypeName());
        component.displayName("Reflection samples (tutorial)");
        component.property("description", &Samples::description).category("About");
        component.object("values", "tutorial.SampleValues", &Samples::values, &Samples::editableValues)
            .displayName("Editable values").category("Objects");
        component.object("preview", "tutorial.SampleValues", &Samples::values)
            .displayName("Read-only view of the same values").category("Objects");
        component.polymorphicObject<SampleMode>("mode", "tutorial.SampleMode",
            &Samples::mode, &Samples::editableMode, &Samples::setMode)
            .displayName("Mode").category("Objects").subtype("smooth").subtype("step");
        component.polymorphicObject<SampleMode>("modePreview", "tutorial.SampleMode", &Samples::mode)
            .displayName("Read-only mode").category("Objects");
        component.action("reset", [](Samples* target)
        {
            target->reset();
        }).displayName("Reset sample values").category("Samples");
        registry.registerType(component.build());
        scene::ComponentFactoryRegistry::get().registerComponent(Samples::staticTypeName(), []
        {
            return new Samples();
        });
        if (!registry.validateObjectDepth())
        {
            throw std::logic_error("Reflection object nesting exceeds the supported depth");
        }
        return true;
    }();
    (void)registered;
}

}
```

Add to `main.cpp`:

```cpp
#include "ReflectionSamplesComponent.hpp"
```

In `Application::init()`, immediately after `registerRotationComponent()`, call:

```cpp
tutorial::registerReflectionSamples();
```

This is the last source change. The final target now includes the copied
`main.cpp` and the three tutorial headers. There are no extra shaders or assets.
After adding a new header to an existing Xcode/IDE configuration, regenerate the
same CMake build tree if necessary so the project's source lists are refreshed.

### Plain structs and callable accessors

`SampleValues` has public data fields, not getter/setter methods. Registering it
with `TypeInfoBuilder<SampleValues>("tutorial.SampleValues")` demonstrates the
explicit-key form of the builder. `accessor<V>()` adapts typed lambdas into the
same erased getter/setter contract as `property()`:

```cpp
values.accessor<float>("weight", [](const SampleValues& object)
{
    return object.weight;
}, [](SampleValues& object, const float& value)
{
    object.weight = value;
}).slider().range(0, 1);
```

Use `property()` when existing member functions already match; use `accessor<V>()`
when adapting fields or custom calculations. The callable getter-only overload
of `accessor<V>()`, like the getter-only `property()` overload, creates a read-only
scalar. The component's `description` is a real example of a read-only property.

`propertyTypeOf<V>()` gives the corresponding `PropertyType` at compile time;
`isScalarPropertyType<V>()` lets generic code ask whether a type is supported as
a scalar. For example:

```cpp
static_assert(bg2e::reflection::isScalarPropertyType<float>());
static_assert(bg2e::reflection::propertyTypeOf<float>() == bg2e::reflection::PropertyType::Float);
static_assert(!bg2e::reflection::isScalarPropertyType<tutorial::SampleValues>());
```

`registerType(builder.build())` explicitly stores the description. The builder's
`build()` returns a reference to its current description, so copy it or register
it before destroying the builder. `propertyAt(index)` and `actionAt(index)` allow
advanced builder-time access to the accumulated descriptors; indices must be
valid. Prefer named fluent builders for ordinary metadata changes. For example,
`values.propertyAt(0).metadata.displayName = "Active"` would change the first
property's label before registration. This is an optional experiment, not a
required change to the finished file.

The fluent methods `.input()`, `.slider()`, `.drag()`, `.checkbox()`, `.combo()`,
`.colorEditor()` and `.angle()` set `PropertyEditor` hints. `.editor(value)` sets
the same enum directly. `.min(value)` and `.max(value)` specify individual bounds;
`.range(min, max)` specifies both. Consumers decide how best to represent hints;
a hint does not alter the stored C++ type or enforce runtime validation.

### Object properties and read-only paths

`object("values", typeKey, constGetter, mutableGetter)` exposes the address of a
contained object. It has no parent setter: edit that object's properties through
its own registered setters. The getter-only `preview` is a read-only view of the
same object, useful for comparing the mutable and const paths side by side.

An Object builder accepts display name, category and tooltip. Numeric editor
hints and scalar widgets are not meaningful for an object as a whole. Register
the child's type under the exact `objectTypeName` used by the parent.

`maxObjectDepth` is currently three: the component is depth zero and its values
object is depth one. `objectChainDepth(typeKey)` and
`validateObjectDepth(&offenders)` can diagnose ordinary Object chains at startup.
The latter optionally fills a `vector<string>` with offending type names. These
queries stop unknown keys/cycles and do not replace the consumer's runtime depth
limit, especially for dynamically chosen polymorphic subtypes.

### Owned polymorphic objects and subtype factories

`SampleMode` is a polymorphic base (it has a virtual destructor). SmoothMode and
StepMode derive from it and have their own values. The registry receives three
separate type descriptions: base, Smooth and Step. C++ inheritance does not
implicitly merge reflection descriptions; the adapter draws base and subtype
metadata explicitly.

`registerSubtype<Base, Derived>(baseKey, key, label, typeKey)` associates a menu
key with the concrete C++ type and a default factory. The Step registration uses
the overload with a custom factory returning `shared_ptr<StepMode>`. The reflected
type key (`tutorial.StepMode`) need not equal the menu key (`step`). Register type
and subtype metadata before drawing; their registration order is otherwise
independent.

The editable `polymorphicObject()` takes a const getter, mutable getter and a
replacement function accepting `shared_ptr<SampleMode>`. Calling `.subtype(key)`
restricts the property to an ordered allow-list. With no allow-list the consumer
uses all registered subtypes for that base. The getter-only `modePreview` has
neither a mutable path nor a replacement function, so it is fully read-only.

Useful registry operations can also be tried without a UI after registration:

```cpp
auto& registry = bg2e::reflection::TypeRegistry::get();
auto sample = registry.createSubtype<tutorial::SampleMode>("tutorial.SampleMode", "smooth");
if (sample)
{
    const auto key = registry.subtypeKey("tutorial.SampleMode", typeid(*sample));
    const auto* subtype = registry.subtype("tutorial.SampleMode", key);
    auto* concrete = registry.subtypeObject("tutorial.SampleMode", key,
        static_cast<void*>(sample.get()));
    if (subtype && concrete)
    {
        const auto* info = registry.type(subtype->typeName);
        // 'concrete' now addresses the type expected by that TypeInfo.
        (void)info;
    }
}
const auto choices = registry.subtypes("tutorial.SampleMode");
```

`subtypeObject()` also has a const-pointer overload. `createSubtype<Base>()` uses
the registered hierarchy's exact C++ base type and returns an empty shared pointer
for unknown keys or incompatible factory contracts. Never dereference an unknown
subtype or interpret its base pointer using unrelated metadata. `subtypeKey()`
uses the dynamic RTTI type to find the stable key; it does not register unknown
C++ subclasses automatically.

Our component's clone copies SampleValues and creates an independent concrete
mode object. It never shares the mutable mode with the original component.
An unknown mode becomes null; the only factories offered here are the two
explicitly registered ones. Replacing a mode selects a fresh default instance;
it does not preserve the previous subtype's settings.

### Paths and resources

The plain `path` field demonstrates `PropertyType::Path`. Calling
`.resource("Images", {"png", "jpg"})` turns a String or Path declaration into a
Resource property while preserving the getter/setter storage contract in
`resourceValueType`. Applying `.resource()` to a float is an error.

`image` stores a filesystem path; `imageName` stores a string and requests
project-relative path handling. In the current ResourcePicker implementation,
that flag computes paths relative to `std::filesystem::current_path()` and falls
back to the selected path if conversion fails. Run from the intended project
working directory if you need that behavior. The flag does not discover a project
root automatically. The sample merely records the selected value; it does not
load textures or modify materials.

`clearableResource()` permits the Clear button. `resourceEmptySelection()` allows
an empty result from the file dialog to be accepted as a value (including a dialog
cancellation); the picker does not offer a separate empty-file option. These are
separate flags, so choose cancellation behavior deliberately in a real tool. Resource metadata includes a human-readable kind, extension filters,
project-relative preference and the two empty/clear flags. Store extensions
without leading dots.

Selecting a resource edits representation data; reflection does not automatically
realize GPU resources. An application that uses reflected paths as actual scene
assets must supply its own loading, validation, rejection/rollback and lifetime
policy. The playground intentionally keeps sample paths independent of rendering.
The inherited material and graphics editors retain their existing resource paths.

### Actions defined by callables and direct factory registration

The sample's Reset action uses `action(name, lambda)` rather than a member-function
pointer. That lambda receives `ReflectionSamplesComponent*`; the builder erases
it into `ActionInfo::invoke`. This is useful for tool behavior that belongs in
registration code. Here it calls `reset()` and leaves the active mode unchanged.

The sample registers its component factory explicitly with
`ComponentFactoryRegistry::registerComponent(key, creator)`. This is the underlying
operation wrapped by `RegisterComponent<T>` in Part 2. The registry's
`createDefault(key)` constructs an instance without scene-file parsing.
Its `create(json, basePath, ...)` overloads belong to component deserialization
and optional resource realization; this tutorial does not call them because its
sample components are deliberately excluded from scene serialization.

## 8. Check the complete application

Build/run `tutorial_reflection` using the commands from Part 1. Then perform
these checks in order:

1. Click Original model. Both node windows show the same name and Rotation
   component. Set its speed to zero in one window and confirm that the other
   shows zero on the next frame. Queued edits in the custom editor appear after
   the next update boundary.
2. Change Axis and Speed in the custom editor. Positive and negative speeds
   must rotate in opposite directions. With a nonzero speed, click Toggle rotation
   in either editor to pause, then click it in the other editor to resume. Speed
   and the current pose must be preserved while paused. Change Speed while paused
   and confirm that resuming uses the newly configured value.
3. Ctrl-click Independent clone. Both editors must keep showing the original
   first-selected model. Deselect that first node and confirm that both switch
   together to the next one. Deselect everything and check both empty states.
4. Add and remove Rotation using either window. Confirm that the other window's
   component list and Add Component options catch up. Re-add Transform if you
   removed it while experimenting; Rotation intentionally tolerates its absence.
5. On a selected model, add `Reflection samples (tutorial)`. Try every scalar
   field under Editable values: checkbox, signed/unsigned numbers, float/double,
   text, vectors, matrix and color. Expand the read-only preview and confirm it
   mirrors changes but cannot be edited. These values do not change the model.
6. Under Mode, switch Smooth to Step, edit its fields and inspect the read-only
   mode preview. Base `active` and derived `smoothing`/`steps` are separate
   registered descriptions. Changing subtype replaces the object, so old
   subtype fields must disappear. Switch back and observe the new defaults.
7. Test the path and both resource fields. The String resource must remain a
   string and the Path resource a filesystem path; the picker must not cause an
   `any_cast` failure. Clear a clearable resource. On macOS use the Xcode build
   so native dialogs work.
8. Click Reset sample values. Confirm that the editable values and their
   read-only preview both reset, while Mode remains unchanged. Delete the node
   through the original application's controls and confirm both windows stop
   displaying it. Queued edits must not resurrect removed nodes/components.
9. Open “Graphics settings” and try the inherited manipulation/material controls.
   They remain the baseline application's controls, independent of the sample
   reflection data. Close the application to exercise the retained cleanup path.

These are manual verification steps for the tutorial reader; no automated build
or runtime verification is implied by the documentation. The full application
is the state reached by completing the three pages, including the optional
sample component's registration and both node windows.

## 9. Keep the reflection layers distinct

| Task | API used in this tutorial |
| --- | --- |
| Describe member accessors | `TypeInfoBuilder::property()` |
| Adapt fields/callables | `TypeInfoBuilder::accessor<V>()` |
| Describe nested ownership | `object()`, `polymorphicObject()` |
| Describe operations | `action()`, `ActionInfo::invoke` |
| Configure presentation | Property/Object/PolymorphicObject/Action builders and metadata |
| Publish a description | `TypeRegistration<T>` or `registerType(builder.build())` |
| Find types/properties/actions | `TypeRegistry::type/contains/typeNames`, `TypeInfo::property/action` |
| Access values | `PropertyInfo::getter/setter/isReadOnly` |
| Construct components | `RegisterComponent<T>`, `ComponentFactoryRegistry` |
| Construct polymorphic values | `registerSubtype`, `createSubtype`, subtype lookup/casts |
| Check object depth | `maxObjectDepth`, `objectChainDepth`, `validateObjectDepth` |
| Build standard widgets | `ReflectionWidget::editValue/drawProperties/drawActions` |
| Bind delayed edits | `ReflectionWidget::EditHandler` and application update queue |

For reference signatures and additional examples, see the
[reflection API index](../../api/reflection/index.md),
[builder reference](../../api/reflection/Builder.md),
[type registry reference](../../api/reflection/TypeRegistry.md) and
[reflection widget reference](../../api/ui/Reflection_Widgets.md).

[Previous: Create a rotating component](02_rotation_component.md) ·
[Start: Scene and manipulation](01_scene_and_manipulation.md)
