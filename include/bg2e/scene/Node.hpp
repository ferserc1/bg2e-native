//
//  Node.hpp
#pragma once

#include <bg2e/common.hpp>

#include <bg2e/scene/Component.hpp>

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>


namespace bg2e {
namespace scene {

class BG2E_API Node {
public:
    Node();
    Node(const std::string& name);
    virtual ~Node();
    
    void addChild(std::shared_ptr<Node> node);
    void removeChild(std::shared_ptr<Node> node);
    const std::vector<std::shared_ptr<Node>>& children() const;
    
    void addComponent(std::shared_ptr<Component> comp);
    void removeComponent(std::shared_ptr<Component> comp);
    const std::unordered_map<std::string, std::shared_ptr<Component>>& components() const;
    
protected:
    std::vector<std::shared_ptr<Node>> _children;
    
    std::unordered_map<std::string, std::shared_ptr<Component>> _components;
    
    std::string _name;
    std::string _identifier;
};

}
}
