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
#include <bg2e/scene/ComponentFactoryRegistry.hpp>
#include <bg2e/utils/utils.hpp>
#include <bg2e/scene/TransformVisitor.hpp>

#include "bg2e/base/Log.hpp"
#include "bg2e/scene/Scene.hpp"

#include <algorithm>

namespace bg2e::scene {

Node::Node()
    :_identifier { utils::uniqueId() }
{

}

Node::Node(const std::string& name)
    :_name { name }
    ,_identifier { utils::uniqueId() }
{

}

Node::~Node()
{

}

void Node::addChild(Node * node)
{
    addChild(std::shared_ptr<Node>(node));
}

void Node::addChild(std::shared_ptr<Node> node)
{
    _children.push_back(node);
    node->_parent = this;
}

void Node::removeChild(const std::shared_ptr<Node> & node)
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

void Node::clearChildren()
{
    _children.clear();
}

void Node::addComponent(Component * comp)
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
    if (!comp) return;

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
    return _orderedComponents;
}

void Node::accept(NodeVisitor * visitor)
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

void Node::acceptReverse(NodeVisitor * visitor)
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
    return glm::normalize(glm::mat3{ wm } * glm::vec3{ 0.0f, 0.0f, 1.0f });
}

glm::vec3 Node::rightVector()
{
    auto wm = worldMatrix();
    return glm::normalize(glm::mat3{ wm } * glm::vec3{ 1.0f, 0.0f, 0.0f });
}

glm::vec3 Node::upVector()
{
    auto wm = worldMatrix();
    return glm::normalize(glm::mat3{wm } * glm::vec3{ 0.0f, 1.0f, 0.0f });
}

Node * Node::sceneRoot()
{
    if (_parent == nullptr)
    {
        return this;
    }
    return _parent->sceneRoot();
}

void Node::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath)
{
    if (!jsonData || !jsonData->isObject())
    {
        return;
    }

    auto& obj = jsonData->objectValue();

    // Read basic node properties
    if (obj.count("name"))
    {
        _name = obj["name"]->stringValue();
    }

    if (obj.count("enabled"))
    {
        _disabled = !obj["enabled"]->boolValue();
    }

    if (obj.count("steady"))
    {
        _steady = obj["steady"]->boolValue();
    }

    // Deserialize components
    if (obj.count("components") && obj["components"]->isList())
    {
        auto& componentsList = obj["components"]->listValue();
        for (auto& compData : componentsList)
        {
            auto* comp = ComponentFactoryRegistry::get().create(compData, basePath);
            if (comp)
            {
                addComponent(comp);
            }
        }
    }

    // Deserialize children recursively
    if (obj.count("children") && obj["children"]->isList())
    {
        auto& childrenList = obj["children"]->listValue();
        for (auto& childData : childrenList)
        {
            auto childNode = std::make_shared<Node>();
            childNode->deserialize(childData, basePath);
            addChild(childNode);
        }
    }
}

std::shared_ptr<json::JsonNode> Node::serialize(const std::filesystem::path & basePath)
{
    using namespace bg2e::json;
    auto result = JSON(JsonObject{});
    auto & obj = result->objectValue();
    
    obj["type"] = JSON("Node");
    obj["name"] = JSON(name());
    obj["enabled"] = JSON(enabled());
    obj["steady"] = JSON(steady());
    
    // Serialize components
    auto components = JSON(JsonList());
    for (auto & comp : _components)
    {
        auto compData = comp.second->serialize(basePath);
        components->listValue().push_back(compData);
    }
    obj["components"] = components;
    
    // Serialize children
    auto children = JSON(JsonList());
    for (auto & child : _children)
    {
        auto childData = child->serialize(basePath);
        children->listValue().push_back(childData);
    }
    obj["children"] = children;
    
    return result;
}

void Node::sortComponents() const
{
    if (_componentsOrderDirty)
    {
        _orderedComponents.clear();
        for (auto& [_, comp] : _components)
        {
            _orderedComponents.push_back(comp);

            if (_orderedComponents.size() > 10)
            {
                std::cout << "Cuidado" << std::endl;
            }
        }
        std::stable_sort(
            _orderedComponents.begin(),
            _orderedComponents.end(),
            [](const auto& a, const auto& b)
            {
                return a->priority() > b->priority();
            }
        );
        _componentsOrderDirty = false;
    }
}
    
}
