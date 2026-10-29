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

#include "EditorUi.hpp"

#include <bg2e/ui/Button.hpp>
#include <bg2e/ui/Text.hpp>
#include <bg2e/ui/Layout.hpp>
#include <bg2e/ui/Value.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/ui/ReflectionWidget.hpp>
#include <bg2e/reflection/Registry.hpp>
#include <typeinfo>
#include <stdexcept>
#include <bg2e/draw/RendererDeferred.hpp>
#include <bg2e/scene/OrbitCameraComponent.hpp>
#include <bg2e/scene/PolarTransformController.hpp>
#include <bg2e/scene/FixedScaleTransformController.hpp>
#include <bg2e/scene/ChainJoint.hpp>

namespace bg2e_composer {

void EditorUi::drawSceneEditor(const std::shared_ptr<AppDelegate>& delegate)
{
    using namespace bg2e;
    auto document = delegate->document();
    if (!document || !document->root || !delegate->selection())
    {
        return;
    }
    const auto weak = _delegate;
    _sceneTree.setRootNode(document->root.get());
    _sceneTree.setSelectionManager(delegate->selection().get());
    _sceneTree.onSelect([weak](auto node, bool additive)
    {
        if (auto owner = weak.lock())
        {
            owner->selectTreeNode(node->shared_from_this(), additive);
        }
    });
    _sceneTree.onReparent([weak](auto node, auto parent)
    {
        if (auto owner = weak.lock())
        {
            owner->reparentNode(node->shared_from_this(), parent->shared_from_this());
        }
    });
    const float treeHeight = ui::Layout::getContentRegionAvailHeight() * 0.5f;
    ui::Layout::beginChild("scene_tree", 0.0f, treeHeight);
    _sceneTree.draw();
    ui::Layout::endChild();
    ui::Layout::beginChild("node_editor");
    auto target = delegate->selection()->selectedNode();
    if (delegate->selection()->selectedNodes().size() > 1)
    {
        ui::Text::text("<multiple_selection>");
        ui::Layout::endChild();
        return;
    }
    if (!target)
    {
        ui::Text::text("No selection");
        ui::Layout::endChild();
        return;
    }
    const auto node = std::weak_ptr<scene::Node>(target->shared_from_this());
    auto name = target->name();
    if (ui::Value::text("Node name", name))
    {
        delegate->renameNode(node, name);
    }
    auto enabled = target->enabled();
    if (ui::Button::checkBox("Enabled", &enabled))
    {
        delegate->setNodeEnabled(target->shared_from_this(), enabled);
    }
    _componentInspector.setNode(target);
    _componentInspector.onAddRequested([weak, node](const std::string& typeName)
    {
        if (auto owner = weak.lock())
        {
            owner->addSceneComponent(node, typeName);
        }
    });
    _componentInspector.onRemoveRequested([weak, node](auto component)
    {
        if (auto owner = weak.lock())
        {
            owner->editComponent(node, component, {});
        }
    });
    _componentInspector.onDrawComponent([weak, node](auto component)
    {
        try
        {
            // Drawable resources have their own left-hand editor.
            if (dynamic_cast<scene::DrawableComponent*>(component.get()))
            {
                ui::Text::text("Edit drawable and materials in Model Properties.");
                return;
            }
            if (const auto* info = reflection::TypeRegistry::get().type(component->typeName()))
            {
                if (auto environment = dynamic_cast<scene::EnvironmentComponent*>(component.get()))
                {
                    if (auto owner = weak.lock())
                    {
                        const bool active = owner->stage()->environmentNode().get() == environment->ownerNode();
                        ui::Text::text(active ?
                            "Active scene environment" : "Inactive environment (not used for rendering)");
                        if (auto deferred = dynamic_cast<draw::RendererDeferred*>(owner->renderer());
                            active && deferred && !deferred->environmentError().empty())
                        {
                            ui::Text::text("Environment error: " + deferred->environmentError());
                        }
                    }
                    if (!environment->environmentImage().empty())
                    {
                        ui::Text::text("Clear Environment Image to use the procedural sky colors.");
                    }
                }
                const ui::ReflectionWidget::EditHandler edit =
                    [weak, node, target = std::weak_ptr<scene::Component>(component)](auto operation, bool resourceChange)
                {
                    if (auto owner = weak.lock())
                    {
                        owner->editComponentProperties(node, target, std::move(operation), resourceChange);
                    }
                };
                ui::ReflectionWidget::drawProperties(component.get(), *info, 0, edit);
                ui::ReflectionWidget::drawActions(component.get(), *info, edit);
            }
        }
        catch (const std::exception& error)
        {
            ui::Text::text(error.what());
        }
    });
    _componentInspector.draw();
    ui::Layout::endChild();
}


void AppDelegate::selectTreeNode(std::weak_ptr<bg2e::scene::Node> target, bool additive)
{
    const auto generation = document() ? document()->generation : 0;
    enqueueRendererUpdate(rendererGeneration(), [weak = weak_from_this(), target, additive, generation](auto&)
    {
        auto owner = weak.lock();
        auto node = target.lock();
        if (!owner || !node || !owner->document() || owner->document()->generation != generation ||
            !owner->_stage->contains(node.get()))
        {
            return;
        }
        auto selection = owner->_selection;
        const bool selected = selection->isSelected(node.get());
        if (!additive)
        {
            selection->deselect();
        }
        if (additive && selected)
        {
            selection->removeFromSelectedItems(node.get());
        }
        else
        {
            selection->addNodeToSelection(node.get());
        }
        owner->requestSceneFrame();
    });
}


void AppDelegate::editComponentProperties(std::weak_ptr<bg2e::scene::Node> node,
    std::weak_ptr<bg2e::scene::Component> component,
    std::function<void(void*)> operation, bool resourceChange)
{
    auto current = component.lock();
    if (!current || !operation)
    {
        return;
    }
    using namespace bg2e::scene;
    const auto& type = typeid(*current);
    // Only known CPU representations opt out of the conservative GPU boundary.
    // A custom component's reflected setters may own native resources.
    const bool cpuValuesOnly = !resourceChange &&
        (type == typeid(TransformComponent) || type == typeid(LightComponent) ||
         type == typeid(CameraComponent) || type == typeid(EnvironmentComponent) ||
         type == typeid(OrbitCameraComponent) || type == typeid(PolarTransformControllerComponent) ||
         type == typeid(FixedScaleTransformControllerComponent) ||
         type == typeid(InputChainJointComponent) || type == typeid(OutputChainJointComponent));
    auto edit = [this, component, operation = std::move(operation)](auto& target)
    {
        auto current = component.lock();
        if (!current || current->ownerNode() != &target)
        {
            return;
        }
        // Changing a light's type can exceed the supported enabled-light count.
        // Keep the original representation for rollback without detaching it.
        auto* light = dynamic_cast<LightComponent*>(current.get());
        const auto previousLight = light
            ? std::optional<bg2e::base::Light>(light->light()) : std::nullopt;
        try
        {
            operation(current.get());
            _stage->validateEditingGraph();
        }
        catch (...)
        {
            if (previousLight)
            {
                light->light() = *previousLight;
            }
            throw;
        }
        // Gizmos detect representation changes in their normal update path;
        // property edits do not require a document-wide tools rebuild.
    };
    if (cpuValuesOnly)
    {
        queueNodeValueEdit(node, std::move(edit));
    }
    else
    {
        queueNodeEdit(node, std::move(edit));
    }
}


void AppDelegate::editComponent(std::weak_ptr<bg2e::scene::Node> node,
    std::weak_ptr<bg2e::scene::Component> original,
    std::shared_ptr<bg2e::scene::Component> replacement)
{
    queueNodeEdit(node, [this, original, replacement](auto& target)
    {
        auto old = original.lock();
        if (!old || old->ownerNode() != &target)
        {
            return;
        }
        const auto* replacementComponent = replacement.get();
        const auto* oldComponent = old.get();
        if (replacementComponent && (replacementComponent->ownerNode() ||
            typeid(*replacementComponent) != typeid(*oldComponent)))
        {
            throw std::invalid_argument("Component replacement must be detached and retain its type");
        }
        if (replacement)
        {
            replacement->setPriority(old->priority());
            replacement->setIgnoreSerialization(old->ignoreSerialization());
        }
        if (replacement && typeid(*oldComponent) == typeid(bg2e::scene::EnvironmentComponent))
        {
            // Keep the authored component identity and inspector/widget state.
            // Only the selected environment is realized by RendererDeferred.
            auto& current = static_cast<bg2e::scene::EnvironmentComponent&>(*old);
            const auto& edited = static_cast<const bg2e::scene::EnvironmentComponent&>(*replacement);
            current.setSkyHorizonColor(edited.skyHorizonColor());
            current.setSkyZenithColor(edited.skyZenithColor());
            current.setGroundColor(edited.groundColor());
            current.setSkyboxBlur(edited.skyboxBlur());
            current.setSkyboxVisible(edited.skyboxVisible());
            current.setEnvironmentImage(edited.environmentImage());
            return;
        }
        auto* scene = _stage->sceneObject();
        const bool activeEnvironment = scene && scene->hasExplicitMainEnvironment() && scene->mainEnvironment() == old.get();
        target.removeComponent(old);
        try
        {
            if (replacement)
            {
                target.addComponent(replacement);
            }
            _stage->validateEditingGraph();
            if (activeEnvironment)
            {
                auto* nextEnvironment = dynamic_cast<bg2e::scene::EnvironmentComponent*>(replacement.get());
                if (nextEnvironment)
                {
                    scene->setMainEnvironment(nextEnvironment);
                }
                else
                {
                    scene->setMainEnvironment(nullptr);
                }
            }
            if (replacement && dynamic_cast<bg2e::scene::EnvironmentComponent*>(old.get()))
            {
                _stage->environmentComponentReplaced(target);
            }
        }
        catch (...)
        {
            if (replacement && replacement->ownerNode() == &target)
            {
                target.removeComponent(replacement);
            }
            target.addComponent(old);
            if (activeEnvironment)
            {
                auto* oldEnvironment = dynamic_cast<bg2e::scene::EnvironmentComponent*>(old.get());
                if (oldEnvironment)
                {
                    scene->setMainEnvironment(oldEnvironment);
                }
                else
                {
                    scene->setMainEnvironment(nullptr);
                }
            }
            if (dynamic_cast<bg2e::scene::EnvironmentComponent*>(old.get()))
            {
                _stage->environmentComponentReplaced(target);
            }
            throw;
        }
        // queueNodeEdit refreshes stale submesh/component handles after rebuilding
        // tools. Keep the authored node selected while editing its inspector.
    }, true);
}


void AppDelegate::addSceneComponent(std::weak_ptr<bg2e::scene::Node> node, const std::string& typeName)
{
    queueNodeEdit(node, [this, typeName](auto& target)
    {
        for (const auto& current : target.orderedComponents())
        {
            if (current->typeName() == typeName)
            {
                return;
            }
        }
        auto component = std::shared_ptr<bg2e::scene::Component>(
            bg2e::scene::ComponentFactoryRegistry::get().createDefault(typeName));
        if (!component || component->ownerNode() || component->typeName() != typeName)
        {
            throw std::invalid_argument("Component factory must return the requested detached type");
        }
        target.addComponent(component);
        try
        {
            _stage->validateEditingGraph();
            component->loadResources(*_engine);
        }
        catch (...)
        {
            target.removeComponent(component);
            throw;
        }
    }, true);
}


void AppDelegate::addSceneItem(unsigned kind)
{
    using namespace bg2e;
    if (!document() || !document()->root || kind > 3)
    {
        return;
    }
    auto selected = _selection->selectedNode();
    auto parent = selected ? selected->shared_from_this() : document()->root;
    queueNodeEdit(parent, [this, kind](auto& target)
    {
        auto child = std::make_shared<scene::Node>(kind == 1 ? "Camera" : kind == 2 ? "Light" : kind == 3 ? "Environment" : "Group");
        child->addComponent(new scene::TransformComponent());
        if (kind == 1)
        {
            child->addComponent(new scene::CameraComponent());
        }
        else if (kind == 2)
        {
            child->addComponent(new scene::LightComponent());
        }
        else if (kind == 3)
        {
            child->addComponent(new scene::EnvironmentComponent());
        }
        target.addChild(child);
        try
        {
            _stage->validateEditingGraph();
        }
        catch (...)
        {
            target.removeChild(child);
            throw;
        }
    }, true);
}


void AppDelegate::addPrimitive(unsigned kind)
{
    if (!document() || !document()->root || kind > 2)
    {
        return;
    }
    auto selected = _selection->selectedNode();
    auto parent = selected ? selected->shared_from_this() : document()->root;
    queueNodeEdit(parent, [this, kind](auto& target)
    {
        auto child = _stage->createPrimitive(kind);
        target.addChild(child);
        try
        {
            _stage->prepareTools(child);
            _stage->validateEditingGraph();
        }
        catch (...)
        {
            target.removeChild(child);
            _stage->discardCandidate(child);
            throw;
        }
    }, true);
}


void AppDelegate::reparentNode(std::weak_ptr<bg2e::scene::Node> target,
    std::weak_ptr<bg2e::scene::Node> destination)
{
    queueNodeEdit(target, [this, destination](auto& node)
    {
        auto parent = destination.lock();
        if (!parent || !_stage->contains(parent.get()) || !node.parent() || &node == document()->root.get())
        {
            return;
        }
        for (auto ancestor = parent.get(); ancestor; ancestor = ancestor->parent())
        {
            if (ancestor == &node)
            {
                throw std::invalid_argument("Cannot reparent a node into its own subtree");
            }
        }
        auto oldParent = node.parent()->shared_from_this();
        auto retained = node.shared_from_this();
        oldParent->removeChild(retained);
        try
        {
            parent->addChild(retained);
            _stage->validateEditingGraph();
        }
        catch (...)
        {
            if (node.parent() == parent.get())
            {
                parent->removeChild(retained);
            }
            oldParent->addChild(retained);
            throw;
        }
        _selection->deselect();
        _selection->addNodeToSelection(&node);
    }, true);
}

}
