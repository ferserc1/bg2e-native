# Part 2 — Create and register a rotating component

[Previous: Scene and manipulation](01_scene_and_manipulation.md) ·
[Next: Build a node editor from reflection](03_reflection_editor.md)

We now add a behavior that belongs to a node: rotate its transform by a chosen
number of degrees each second around its local X, Y or Z axis. Reflection will
let the built-in editor discover these settings without a special rotation
widget or a change to engine code.

## 1. Separate behavior, description and construction

Three independent mechanisms cooperate:

| Mechanism | What it provides |
| --- | --- |
| `scene::Component` | Ownership, animation callbacks and the component's behavior. |
| `reflection::TypeRegistry` | Descriptions of properties/actions and functions to access them. |
| `scene::ComponentFactoryRegistry` | A constructor that lets the editor add an instance by type name. |

Reflection here is explicit C++ registration, not automatic discovery of class
members. An ordinary getter is invisible until its metadata is registered. A
registered type does not necessarily have a component constructor: reflected
objects can also be plain structs, as we will see in Part 3.

Our stable type key is `tutorial.Rotation`; its user-facing display name is
`Rotation (tutorial)`. They serve different purposes. The key connects the
component, metadata and factory. The display name can change without breaking
that connection. Do not use compiler-specific RTTI names as persistent keys.

## 2. Define the component

Create `examples/tutorials/reflection/src/RotationComponent.hpp`. Start with the
following code, including the project's full GPL comment. The last namespace
brace will be supplied in Step 4, after the registration function:

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

#include <bg2e/scene/Node.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/reflection/Registration.hpp>

#include <algorithm>
#include <cmath>
#include <memory>

namespace tutorial {

class RotationComponent final : public bg2e::scene::Component {
  public:
    BG2E_COMPONENT_TYPE_NAME("tutorial.Rotation");

    enum class Axis
    {
        X,
        Y,
        Z
    };

    RotationComponent()
    {
        // This example edits live scenes; it does not implement scene-file IO.
        setIgnoreSerialization(true);
    }

    Axis axis() const
    {
        return _axis;
    }
    void setAxis(Axis value)
    {
        if (value == Axis::X || value == Axis::Y || value == Axis::Z)
        {
            _axis = value;
        }
    }
    float speed() const
    {
        return _speed;
    }
    void setSpeed(float value)
    {
        if (std::isfinite(value))
        {
            _speed = std::clamp(value, -360.0f, 360.0f);
        }
    }

    void toggleRotation()
    {
        _rotating = !_rotating;
    }

    void animate(float deltaSeconds) override
    {
        auto* node = ownerNode();
        auto* transform = node ? node->transform() : nullptr;
        if (!_rotating || !transform || !std::isfinite(deltaSeconds) || deltaSeconds <= 0.0f)
        {
            return;
        }
        glm::vec3 direction(0.0f);
        switch (_axis)
        {
        case Axis::X:
            direction.x = 1.0f;
            break;
        case Axis::Y:
            direction.y = 1.0f;
            break;
        case Axis::Z:
            direction.z = 1.0f;
            break;
        }
        transform->rotate(glm::radians(_speed * deltaSeconds), direction);
    }

    std::shared_ptr<bg2e::scene::Component> clone() const override
    {
        auto copy = std::make_shared<RotationComponent>();
        copy->setAxis(_axis);
        copy->setSpeed(_speed);
        copy->_rotating = _rotating;
        copy->setPriority(priority());
        copy->setIgnoreSerialization(ignoreSerialization());
        return copy;
    }

  private:
    Axis _axis = Axis::Y;
    float _speed = 45.0f;
    bool _rotating = true;
};
```

`BG2E_COMPONENT_TYPE_NAME` supplies both `staticTypeName()` and the virtual
`typeName()`. The former is useful before an instance exists; the latter lets
an inspector discover the metadata for a component through its base pointer.

`Axis` is an enum, not an arbitrary vector. This restricts the interface to the
three supported directions. `setAxis()` also rejects invalid enum values supplied
by a generic caller. `setSpeed()` rejects non-finite numbers and clamps the value
to our chosen application limit of ±360 degrees/second. UI range metadata alone
is not a substitute for validation in the setter: C++ callers and other tools
can bypass a widget.

The default is Y at 45 degrees/second. Negative speed reverses rotation. Zero
produces no movement without resetting the object's pose. `toggleRotation()` is a
parameterless action that switches the independent `_rotating` flag: the first
click pauses rotation, and the next resumes it at the configured speed. The flag
starts as `true`; toggling never changes the speed, axis or current transform.
If speed is zero, resuming still produces no movement until you choose a nonzero
speed.

## 3. Understand the animation callback and ownership

The engine's `scene::UpdateVisitor` visits enabled nodes. For each node it calls
its components' `animate(delta)` methods, then their `update(delta)` methods.
The deferred renderer runs that visitor using the frame's scene delta in seconds.
Do not add a second visitor call in our render delegate: that would advance
animation twice. Keep `setContinuousRendering(true)` from the starting example.

The angle added in one frame is:

```text
angle in degrees = speed in degrees/second × elapsed seconds
angle in radians = radians(angle in degrees)
```

While `_rotating` is false, `animate()` returns without applying a rotation.
The component looks up its owner's transform on every animation call. A node
can exist without a transform, and the node editor can remove one, so we check
for null and do nothing in that case. The component does not create hidden
transforms. To resume rotation after removing Transform, add Transform again.

`TransformComponent::rotate()` takes radians and appends an incremental rotation
to the current matrix. In this example that means rotation around a local axis,
keeping the current translation. We use `rotate()` rather than `setRotation()`
so that each frame builds on the current transform instead of replacing it.
With nonuniform scaling, repeated matrix multiplication can introduce shear;
use the unscaled original model for this exercise. General transform controllers
need a deliberately chosen decomposition/composition policy.

Every component must implement `clone()`. A clone must be detached and independent:
copying a component's base object can accidentally copy its borrowed owner pointer.
Our implementation constructs a fresh component and copies the settings, the
running/paused state and common flags. The new component acquires an owner only when added to a node.

`setIgnoreSerialization(true)` makes this a live tutorial behavior rather than a
scene-file feature. Reflection does not automatically implement
`serializeReferences()`/`deserialize()` or resource loading. This application does
not save scenes. If you later add scene persistence, implement that separate
contract explicitly before opting this component into serialization.

## 4. Register the properties and action

Append this code to the same header, including its final namespace brace:

```cpp
inline void registerRotationComponent()
{
    // Function-local statics register once, explicitly before application startup.
    static bg2e::reflection::TypeRegistration<RotationComponent> reflectionRegistration(
        [](bg2e::reflection::TypeInfoBuilder<RotationComponent>& type)
        {
            type.displayName("Rotation (tutorial)");
            type.property("axis", &RotationComponent::axis, &RotationComponent::setAxis)
                .displayName("Axis").category("Rotation").combo()
                .enumValue("X", RotationComponent::Axis::X)
                .enumValue("Y", RotationComponent::Axis::Y)
                .enumValue("Z", RotationComponent::Axis::Z)
                .tooltip("Rotate around this local axis");
            type.property("speed", &RotationComponent::speed, &RotationComponent::setSpeed)
                .displayName("Speed (degrees/second)").category("Rotation")
                .drag().range(-360.0, 360.0).step(1.0)
                .tooltip("Negative values reverse rotation; zero pauses it");
            type.action("toggleRotation", &RotationComponent::toggleRotation)
                .displayName("Toggle rotation").category("Rotation")
                .tooltip("Pause or resume rotation without changing the speed or transform");
        });
    static bg2e::scene::RegisterComponent<RotationComponent> factoryRegistration;
    (void)reflectionRegistration;
    (void)factoryRegistration;
}

}
```

The function is `inline` because it is defined in a header. Its function-local
static registrations run only once when we explicitly call it at startup. No
relative initialization order between different translation units is required.

`TypeRegistration<RotationComponent>` creates a `TypeInfoBuilder` using
`RotationComponent::staticTypeName()`, executes our definition lambda and copies
the finished `TypeInfo` into `TypeRegistry`. The builder itself can go out of scope;
the registry owns the metadata copy. The overload taking an explicit string is
useful for plain types without `staticTypeName()`.

Each `property(name, getter, setter)` creates a `PropertyInfo` containing:

- A machine-readable name (`axis` or `speed`) and inferred `PropertyType`.
- A type-erased getter, `std::any(const void*)`.
- A type-erased setter, `void(void*, const std::any&)`.
- Editor hints and presentation metadata.

Type erasure lets an inspector handle unrelated C++ classes through the same API.
It does not remove type requirements: the pointer must address the exact type
expected by the metadata, and `std::any` must contain the exact expected value
type. A wrong `any_cast` throws `std::bad_any_cast`.

For a `float` property, the erased value is a `float`, not a `double`. For any
C++ enum registered through the builder, the erased value is **`int64_t`**, not
the enum type and not the combo-box index. The builder converts it back to `Axis`
when calling the setter. `.enumValue(label, value)` supplies explicit label/value
pairs; never infer enum values from their positions in a list.

The metadata calls have these effects:

| Call | Meaning |
| --- | --- |
| `.displayName(...)` | Human-readable label; the stable property name remains unchanged. |
| `.category("Rotation")` | Groups related controls. |
| `.tooltip(...)` | Help text for a widget. |
| `.combo()` | Requests a choice list. |
| `.enumValue(...)` | Adds one label/value option, in display order. |
| `.drag()` | Requests a draggable number field. |
| `.range(-360, 360)` | Provides minimum and maximum hints. |
| `.step(1)` | Provides the drag sensitivity. |

Do not use `.angle()` for speed: our units are degrees per second, rather than an
orientation angle. `.angle()` is useful for an actual angular property such as
Transform's Euler rotation.

`action("toggleRotation", &RotationComponent::toggleRotation)` creates an `ActionInfo` with a
parameterless type-erased invoker. The action can have a display name, category
and tooltip. Unlike a property, an action describes an operation rather than a
stored value.

`scene::RegisterComponent<RotationComponent>` registers a default constructor in
the separate component factory registry. The built-in editor's Add Component
list includes a type only when reflection metadata **and** a component factory
are present and the node does not already contain that type. Missing either
registration is a common reason for a component not appearing.

## 5. Call registration before creating the application delegates

In `main.cpp`, add the private-header include after the engine includes:

```cpp
#include "RotationComponent.hpp"
```

At the beginning of `Application::init()`, before creating `SceneDelegate`, add:

```cpp
tutorial::registerRotationComponent();
```

This ensures metadata and the factory exist before either the scene or its UI
needs them. The class lives in the application's `tutorial` namespace; no engine
header, source file or API is changed.

## 6. Give one model the behavior initially

In `SceneDelegate::createScene()`, find the call that creates `Original model`:

```cpp
addDrawable(nested, "Original model", model, {-3, 0, 0});
```

Replace it with:

```cpp
auto rotatingModel = addDrawable(nested, "Original model", model, {-3, 0, 0});
rotatingModel->addComponent(new tutorial::RotationComponent());
```

`addDrawable()` returns the newly created node, which already contains Transform,
Drawable and the selection components. The rotation component therefore modifies
that node's existing local transform. Other objects keep their original behavior
until you add Rotation to them through the editor.

## 7. Inspect the metadata directly

You can inspect a component without drawing any UI. For example, while a live
`RotationComponent rotation` is in scope and after registration:

```cpp
auto& registry = bg2e::reflection::TypeRegistry::get();
const auto* info = registry.type(tutorial::RotationComponent::staticTypeName());
if (info)
{
    if (const auto* speed = info->property("speed"))
    {
        auto oldValue = std::any_cast<float>(speed->getter(&rotation));
        speed->setter(&rotation, std::any(oldValue + 5.0f));
    }
    if (const auto* toggle = info->action("toggleRotation"))
    {
        toggle->invoke(&rotation); // Pause a newly constructed component.
        toggle->invoke(&rotation); // Resume with the same speed.
    }
}
```

This is an isolated API experiment, not an extra animation callback. Include
`<any>` when using it. When the instance belongs to the live rendered scene,
route writes through the queued edit mechanism introduced in Part 3.
`TypeInfo::property()` and `action()` return null for unknown names; lookup does
not create entries. A property without a setter is read-only, as reported by
`PropertyInfo::isReadOnly()`.

## 8. Check the second milestone

Select Original model. It rotates immediately. Open `Rotation (tutorial)` in the
built-in editor, change Axis to X or Z and adjust Speed. Try +90, -90 and zero.
Choose a nonzero speed and click Toggle rotation: the model must pause while
Speed keeps its value. Click it again: the model must resume from its current
pose at that same speed. Also change Speed while paused, then resume to confirm
that the new speed takes effect. The Transform panel lets you observe the
changing orientation.

Select Independent clone. Choose `Rotation (tutorial)` from Add Component, add
it and compare the two independently editable speeds. The component should not
be offered a second time on the same node. Remove Rotation and confirm movement
stops; removing the behavior leaves the current transform intact.

Disable a rotating node: the scene update visitor skips it. Re-enable it from
the editor to resume animation. Do not confuse disabling a node with setting
speed to zero; the former also affects its other scene behavior and rendering.

At this point the built-in editor understands our new component entirely from
registered descriptions. We will now implement the same discovery and editing
flow in a second application-owned window.

[Previous: Scene and manipulation](01_scene_and_manipulation.md) ·
[Next: Build a node editor from reflection](03_reflection_editor.md)
