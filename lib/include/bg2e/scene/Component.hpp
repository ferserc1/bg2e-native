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
#include <bg2e/math/projections.hpp>
#include <bg2e/app/KeyEvent.hpp>
#include <bg2e/json/JsonNode.hpp>

#include <string>
#include <filesystem>

#define BG2E_SCENE_COMP_CLASS_NAME(c) typeid(*c).name()

namespace bg2e {
namespace scene {

class Node;
class Scene;

class BG2E_API Component : public std::enable_shared_from_this<Component> {
    friend class Node;
public:
    
    virtual ~Component() = default;
    
    inline Node * ownerNode() const { return _owner; }
    Scene * scene();

    virtual void resizeViewport(const math::Viewport&) {}
    
    virtual void animate(float /* delta */) {}
    
    virtual void update(float /* delta */) {}
    
    virtual void mouseMove(int /* x */, int /* y */) {}

    virtual void mouseButtonDown(int /* button */, int /* x */, int /* y */) {}

    virtual void mouseButtonUp(int /* button */, int /* x */, int /* y */) {}

    virtual void mouseWheel(int /* deltaX */, int /* deltaY */) {}

    virtual void keyDown(const app::KeyEvent& /* keyEvent */) {}
    
    virtual void keyUp(const app::KeyEvent&  /* keyEvent */) {}
    
    virtual std::string typeName() const = 0;
    
    virtual void deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath);
    virtual std::shared_ptr<json::JsonNode> serialize(const std::filesystem::path& basePath);

protected:

    // Weak ptr to avoid circular references
    Node * _owner = nullptr;

    virtual void willAddToNode(Node *) {}
    virtual void didAddToNode(Node *) {}
    virtual void willRemoveFromNode(Node *) {}
    virtual void didRemoveFromNode(Node *) {}
};

extern BG2E_API std::string componentName(Component * comp);

#define BG2E_COMPONENT_TYPE_NAME(typeNameString) \
    static std::string staticTypeName() { return typeNameString; } \
    std::string typeName() const override { return typeNameString; }


}
}
