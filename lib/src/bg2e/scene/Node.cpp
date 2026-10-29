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

#include <bg2e/scene/Node.hpp>
#include <bg2e/json/NodeReader.hpp>
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/utils/utils.hpp>
#include <bg2e/scene/TransformVisitor.hpp>

#include <bg2e/base/Log.hpp>
#include <bg2e/scene/Scene.hpp>

#include <algorithm>
#include <stdexcept>
#include <typeinfo>
#include <unordered_set>

namespace bg2e::scene {

Node::Node()
    : _identifier{utils::uniqueId()}
{
}

Node::Node(const std::string& name)
    : _name{name}, _identifier{utils::uniqueId()}
{
}

Node::~Node()
{
}

void Node::addChild(Node* node)
{
    addChild(std::shared_ptr<Node>(node));
}

void Node::addChild(std::shared_ptr<Node> node)
{
    _children.push_back(node);
    node->_parent = this;
}

void Node::setChildLists(std::vector<ChildList> lists)
{
    std::unordered_set<Node*> parents;
    std::unordered_map<Node*, Node*> proposed;
    for (const auto& list : lists)
    {
        if (!list.parent || !parents.insert(list.parent.get()).second)
        {
            throw std::invalid_argument("Child placement requires unique live parents");
        }
    }
    for (const auto& list : lists)
    {
        for (const auto& child : list.children)
        {
            if (!child || child->_scene || child == list.parent || !proposed.emplace(child.get(), list.parent.get()).second ||
                (child->_parent && !parents.contains(child->_parent)))
            {
                throw std::invalid_argument("Invalid or multiply owned child placement");
            }
        }
    }
    // Removed children become detached in the proposed graph.
    for (const auto& list : lists)
    {
        for (const auto& child : list.parent->_children)
        {
            proposed.try_emplace(child.get(), nullptr);
        }
    }
    for (const auto& list : lists)
    {
        std::unordered_set<Node*> visited;
        for (auto* node = list.parent.get(); node;)
        {
            if (!visited.insert(node).second)
            {
                throw std::invalid_argument("Child placement would create a cycle");
            }
            auto found = proposed.find(node);
            node = found == proposed.end() ? node->_parent : found->second;
        }
    }
    // All publication below is allocation-free and invokes no component hooks.
    for (const auto& list : lists)
    {
        for (const auto& child : list.parent->_children)
        {
            child->_parent = nullptr;
        }
    }
    for (auto& list : lists)
    {
        list.parent->_children.swap(list.children);
        for (const auto& child : list.parent->_children)
        {
            child->_parent = list.parent.get();
        }
    }
}


void Node::setComponentsOrdered(const std::vector<std::shared_ptr<Component>>& components)
{
    std::unordered_map<std::string, std::shared_ptr<Component>> next;
    auto order = components;
    for (const auto& component : components)
    {
        const auto* instance = component.get();
        if (!component || (component->_owner && component->_owner != this) ||
            !next.emplace(typeid(*instance).name(), component).second)
        {
            throw std::invalid_argument("Component placement requires unique concrete types and valid owners");
        }
    }
    // Prepare order and hook lists before changing membership.
    std::stable_sort(order.begin(), order.end(), [](const auto& a, const auto& b)
    {
        return a->priority() > b->priority();
    });
    std::vector<std::shared_ptr<Component>> removed, added;
    for (const auto& old : orderedComponents())
    {
        if (std::find(components.begin(), components.end(), old) == components.end())
        {
            removed.push_back(old);
        }
    }
    for (const auto& component : components)
    {
        if (component->_owner != this)
        {
            added.push_back(component);
        }
    }
    for (const auto& component : removed)
    {
        component->willRemoveFromNode(this);
    }
    for (const auto& component : added)
    {
        component->willAddToNode(this);
    }
    _components.swap(next);
    _orderedComponents.swap(order);
    _componentsOrderDirty = false;
    for (const auto& component : removed)
    {
        component->_owner = nullptr;
    }
    for (const auto& component : added)
    {
        component->_owner = this;
    }
    // Membership remains inspectable if a callback throws; the command owner
    // reapplies its retained previous placement and reports failed compensation.
    for (const auto& component : removed)
    {
        component->didRemoveFromNode(this);
    }
    for (const auto& component : added)
    {
        component->didAddToNode(this);
    }
}


void Node::removeChild(const std::shared_ptr<Node>& node)
{
    if (!node)
    {
        return;
    }

    auto it = std::find(_children.begin(), _children.end(), node);
    if (it != _children.end())
    {
        _children.erase(it);
    }

    node->_parent = nullptr;
    auto root = sceneRoot();
    if (root && root->scene())
    {
        root->scene()->updateLights();
    }
}

const std::vector<std::shared_ptr<Node>>& Node::children() const
{
    return _children;
}

std::shared_ptr<Node> Node::clone() const
{
    // A freshly constructed node already gets its own unique identifier; only the
    // user-visible state (name, enabled/steady flags) is carried over.
    auto copy = std::make_shared<Node>(_name);
    copy->_disabled = _disabled;
    copy->_steady = _steady;

    // Deep copy every component. Component::clone() returns an unowned, independent
    // copy, so the new node shares no component state with this one.
    for (const auto& comp : orderedComponents())
    {
        auto component = comp->clone();
        const auto* clonedComponent = component.get();
        const auto* originalComponent = comp.get();
        if (!clonedComponent || clonedComponent == originalComponent || clonedComponent->ownerNode() ||
            typeid(*clonedComponent) != typeid(*originalComponent))
        {
            throw std::logic_error("Component clone must preserve its concrete type and return a detached independent instance: " + comp->typeName());
        }
        component->setPriority(comp->priority());
        component->setIgnoreSerialization(comp->ignoreSerialization());
        copy->addComponent(std::move(component));
    }

    // Recursively deep copy the child subtree.
    for (const auto& child : _children)
    {
        copy->addChild(child->clone());
    }

    return copy;
}

void Node::clearChildren()
{
    _children.clear();
}

void Node::addComponent(Component* comp)
{
    addComponent(std::shared_ptr<Component>(comp));
}

void Node::addComponent(std::shared_ptr<Component> comp)
{
    if (!comp.get())
    {
        bg2e_log_warning << "Trying to add null component to node " << name() << bg2e_log_end;
        return;
    }
    auto prevOwner = comp->_owner;
    if (prevOwner)
    {
        comp->willRemoveFromNode(prevOwner);
    }
    comp->_owner = nullptr;
    if (prevOwner)
    {
        comp->didRemoveFromNode(prevOwner);
    }

    comp->willAddToNode(this);
    auto plainPtr = comp.get();
    _components[BG2E_SCENE_COMP_CLASS_NAME(plainPtr)] = comp;
    _componentsOrderDirty = true;
    comp->_owner = this;
    comp->didAddToNode(this);

    sortComponents();
}

void Node::removeComponent(std::shared_ptr<Component> comp)
{
    if (!comp)
    {
        return;
    }

    comp->willRemoveFromNode(this);
    auto plainPtr = comp.get();
    auto it = _components.find(BG2E_SCENE_COMP_CLASS_NAME(plainPtr));
    if (it != _components.end())
    {
        _components.erase(it);
        _componentsOrderDirty = true;

        comp->_owner = nullptr;

        comp->didRemoveFromNode(this);
    }

    sortComponents();
}

const std::unordered_map<std::string, std::shared_ptr<Component>>& Node::components() const
{
    return _components;
}

const std::vector<std::shared_ptr<Component>>& Node::orderedComponents() const
{
    sortComponents();
    return _orderedComponents;
}

void Node::accept(NodeVisitor* visitor)
{
    if (!visitor->ignoreDisabled() || !_disabled)
    {
        visitor->visit(this);
        for (auto child : _children)
        {
            if (visitor->cancel())
            {
                return;
            }
            child->accept(visitor);
        }
        if (visitor->cancel())
        {
            return;
        }
        visitor->didVisit(this);
    }
}

void Node::acceptReverse(NodeVisitor* visitor)
{
    if (!visitor->ignoreDisabled() || !_disabled)
    {
        if (_parent)
        {
            _parent->acceptReverse(visitor);
        }
        visitor->visit(this);
    }
}

glm::mat4 Node::worldMatrix()
{
    TransformVisitor visitor;
    return visitor.getWorldMatrix(this);
}

glm::mat4 Node::invertedWorldMatrix()
{
    return glm::inverse(worldMatrix());
}

glm::vec3 Node::worldPosition()
{
    return glm::vec3(worldMatrix()[3]);
}

glm::vec3 Node::forwardVector()
{
    auto wm = worldMatrix();
    return glm::normalize(glm::mat3{wm} * glm::vec3{0.0f, 0.0f, 1.0f});
}

glm::vec3 Node::rightVector()
{
    auto wm = worldMatrix();
    return glm::normalize(glm::mat3{wm} * glm::vec3{1.0f, 0.0f, 0.0f});
}

glm::vec3 Node::upVector()
{
    auto wm = worldMatrix();
    return glm::normalize(glm::mat3{wm} * glm::vec3{0.0f, 1.0f, 0.0f});
}

Node* Node::sceneRoot()
{
    if (_parent == nullptr)
    {
        return this;
    }
    return _parent->sceneRoot();
}

void Node::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath, SceneLoadProgress* progress)
{
    json::ObjectReader reader(jsonData);
    if (!reader.isValid())
    {
        throw std::invalid_argument("Node representation must be a JSON object");
    }

    // Read basic node properties
    if (auto name = reader.getString("name"))
    {
        _name = *name;
    }

    // Fire callback before the heavy component/child work
    if (progress && progress->callback)
    {
        progress->callback(_name, progress->loaded, progress->total);
    }
    if (progress)
    {
        ++progress->loaded;
    }

    if (auto enabled = reader.getBool("enabled"))
    {
        _disabled = !*enabled;
    }
    if (auto steady = reader.getBool("steady"))
    {
        _steady = *steady;
    }

    // Deserialize components
    if (auto components = reader.getArray("components"))
    {
        for (std::size_t i = 0; i < components->size(); ++i)
        {
            auto compData = components->getObject(i);
            if (!compData)
            {
                bg2e_log_warning << "Skipping invalid component at index " << i << " in node " << _name << bg2e_log_end;
                continue;
            }
            auto* comp = ComponentFactoryRegistry::get().create(compData->node(), basePath);
            if (comp)
            {
                addComponent(std::shared_ptr<Component>(comp));
            }
        }
    }

    // Deserialize children recursively
    if (auto children = reader.getArray("children"))
    {
        for (std::size_t i = 0; i < children->size(); ++i)
        {
            auto childData = children->getObject(i);
            if (!childData)
            {
                bg2e_log_warning << "Skipping invalid child at index " << i << " in node " << _name << bg2e_log_end;
                continue;
            }
            auto childNode = std::make_shared<Node>();
            childNode->deserialize(childData->node(), basePath, progress);
            addChild(childNode);
        }
    }
}

void Node::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath,
    draw::Engine& engine, SceneLoadProgress* progress)
{
    deserialize(jsonData, basePath, progress);
    loadResources(engine, progress);
}

void Node::loadResources(draw::Engine& engine, SceneLoadProgress* progress)
{
    for (const auto& entry : _components)
    {
        entry.second->loadResources(engine, progress);
    }
    for (const auto& child : _children)
    {
        child->loadResources(engine, progress);
    }
}

std::shared_ptr<json::JsonNode> Node::serializeReferences(const std::filesystem::path& basePath,
                                                SceneSaveProgress* progress)
{
    // Fire callback before serializing this node
    if (progress && progress->callback)
    {
        progress->callback(_name, progress->saved, progress->total);
    }
    if (progress)
    {
        ++progress->saved;
    }

    using namespace bg2e::json;
    auto result = JSON(JsonObject{});
    auto& obj = result->objectValue();

    obj["type"] = JSON("Node");
    obj["name"] = JSON(name());
    obj["enabled"] = JSON(enabled());
    obj["steady"] = JSON(steady());

    // Serialize components (unchanged)
    auto components = JSON(JsonList());
    for (auto& comp : _components)
    {
        if (comp.second->ignoreSerialization())
        {
            continue;
        }
        auto compData = comp.second->serializeReferences(basePath);
        if (!compData || !json::ObjectReader(compData).isValid())
        {
            bg2e_log_warning << "Skipping empty component serialization for " << comp.second->typeName()
                << " in node " << _name << bg2e_log_end;
            continue;
        }
        components->listValue().push_back(compData);
    }
    obj["components"] = components;

    // Serialize children — forward the same progress pointer
    auto children = JSON(JsonList());
    for (auto& child : _children)
    {
        auto childData = child->serializeReferences(basePath, progress);
        children->listValue().push_back(childData);
    }
    obj["children"] = children;

    return result;
}

void Node::sortComponents() const
{
    if (_componentsOrderDirty)
    {
        std::erase_if(_orderedComponents, [&](const auto& component)
        {
            const auto* value = component.get();
            auto found = _components.find(typeid(*value).name());
            return found == _components.end() || found->second != component;
        });
        for (const auto& entry : _components)
        {
            if (std::find(_orderedComponents.begin(), _orderedComponents.end(), entry.second) == _orderedComponents.end())
            {
                _orderedComponents.push_back(entry.second);
            }
        }
        std::stable_sort(
            _orderedComponents.begin(),
            _orderedComponents.end(),
            [](const auto& a, const auto& b)
            {
                return a->priority() > b->priority();
            });
        _componentsOrderDirty = false;
    }
}

}
