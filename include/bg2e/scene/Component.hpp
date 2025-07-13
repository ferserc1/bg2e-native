//
//  Component.hpp
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/math/projections.hpp>
#include <bg2e/app/KeyEvent.hpp>
#include <bg2e/json/JsonNode.hpp>

#include <string>

#define BG2E_SCENE_COMP_CLASS_NAME(c) typeid(*c).name()

namespace bg2e {
namespace scene {

class Node;
class Scene;

class BG2E_API Component {
    friend class Node;
public:
    
    virtual ~Component() = default;
    
    inline Node * ownerNode() const { return _owner; }
    Scene * scene();

    virtual void resizeViewport(const math::Viewport&) {}
    
    // Use this function to update animation data.
    virtual void animate(float delta) {}
    
    // Use this function to update matrixes generated from
    // the animation data.
    virtual void update(float delta) {}
    
    virtual void mouseMove(int x, int y) {}
    virtual void mouseButtonDown(int button, int x, int y) {}
    virtual void mouseButtonUp(int button, int x, int y) {}
    virtual void mouseWheel(int deltaX, int deltaY) {}
    virtual void keyDown(const app::KeyEvent& event) {}
    virtual void keyUp(const app::KeyEvent& event) {}
    
    virtual std::string typeName() const = 0;
    
    virtual void deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::string& basePath) {}
    virtual std::shared_ptr<json::JsonNode> setialize() { return std::make_shared<json::JsonNode>(); }

protected:

    // Weak ptr to avoid circular references
    Node * _owner = nullptr;
    
    virtual void removedFromNode(Node * prevOwner) {}
    virtual void addedToNode(Node * newOwner) {}
};

extern BG2E_API std::string componentName(Component * comp);

#define BG2E_COMPONENT_TYPE_NAME(typeNameString) \
    static std::string staticTypeName() { return typeNameString; } \
    std::string typeName() const override { return typeNameString; }


}
}
