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

#include <bg2e/common.hpp>

#include <bg2e/scene/Component.hpp>
#include <bg2e/scene/NodeVisitor.hpp>
#include <bg2e/json/JsonNode.hpp>

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <filesystem>

#include <bg2e/scene/TransformComponent.hpp>
#include <bg2e/scene/DrawableComponent.hpp>
#include <bg2e/scene/CameraComponent.hpp>
#include <bg2e/scene/EnvironmentComponent.hpp>
#include <bg2e/scene/LightComponent.hpp>

namespace bg2e {
namespace scene {

class Scene;

class BG2E_API Node : public std::enable_shared_from_this<Node> {
    friend class Scene;
public:
    Node();
    Node(const std::string& name);
    virtual ~Node();
    
    inline const std::string& name() const { return _name; }
    inline void setName(const std::string & name) { _name = name; }
    inline const std::string& identifier() const { return _identifier; }
    inline bool disabled() const { return _disabled; }
    inline void setDisabled(bool d = true) { _disabled = d; }
    inline bool enabled() const { return !_disabled; }
    inline void setEnabled(bool e = true) { _disabled = !e; }
    inline bool steady() const { return _steady; }
    inline void setSteady(bool s) { _steady = s; }
    
    
    void addChild(Node * node);
    void addChild(std::shared_ptr<Node> node);
    void removeChild(std::shared_ptr<Node> node);
    const std::vector<std::shared_ptr<Node>>& children() const;
    void clearChildren();
    
    Node * parent() { return _parent; }
    
    void addComponent(Component * comp);
    void addComponent(std::shared_ptr<Component> comp);
    void removeComponent(std::shared_ptr<Component> comp);
    const std::unordered_map<std::string, std::shared_ptr<Component>>& components() const;
    
    template <typename ComponentT>
    ComponentT * getComponent()
    {
        auto className = typeid(ComponentT).name();
        if (_components.find(className) != _components.end())
        {
            return dynamic_cast<ComponentT*>(_components[className].get());
        }
        return nullptr;
    }
    
    inline TransformComponent * transform() { return getComponent<TransformComponent>(); }
    inline DrawableComponent * drawable() { return getComponent<DrawableComponent>(); }
    inline CameraComponent * camera() { return getComponent<CameraComponent>(); }
    inline EnvironmentComponent * environment() { return getComponent<EnvironmentComponent>(); }
    inline LightComponent * light() { return getComponent<LightComponent>(); }
    
    void accept(NodeVisitor * visitor);
    void acceptReverse(NodeVisitor * visitor);
    
    glm::mat4 worldMatrix();
    glm::mat4 invertedWorldMatrix();
    glm::vec3 worldPosition();
    glm::vec3 forwardVector();
    glm::vec3 rightVector();
    glm::vec3 upVector();

    Node * sceneRoot();
   
    inline Scene * scene() { return sceneRoot()->_scene; }
    
    void deserialize(std::shared_ptr<json::JsonNode>, const std::filesystem::path&);
    std::shared_ptr<json::JsonNode> serialize(const std::filesystem::path&);

protected:
    std::vector<std::shared_ptr<Node>> _children;
    Node * _parent = nullptr;
    Scene * _scene = nullptr;
    
    std::unordered_map<std::string, std::shared_ptr<Component>> _components;
    
    std::string _name;
    std::string _identifier;
    bool _disabled = false;
    bool _steady = false;
};

}
}
