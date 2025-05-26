//
//  Node.hpp
#pragma once

#include <bg2e/common.hpp>

#include <bg2e/scene/Component.hpp>
#include <bg2e/scene/NodeVisitor.hpp>

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
    
    inline const std::string& name() const { return _name; }
    inline void setName(const std::string & name) { _name = name; }
    inline const std::string& identifier() const { return _identifier; }
    inline bool disabled() const { return _disabled; }
    inline void setDisabled(bool d) { _disabled = d; }
    
    
    void addChild(std::shared_ptr<Node> node);
    void removeChild(std::shared_ptr<Node> node);
    const std::vector<std::shared_ptr<Node>>& children() const;
    
    Node * parent() { return _parent; }
    
    void addComponent(std::shared_ptr<Component> comp);
    void removeComponent(std::shared_ptr<Component> comp);
    const std::unordered_map<size_t, std::shared_ptr<Component>>& components() const;
    
    template <typename ComponentT>
    ComponentT * getComponent()
    {
        auto hashCode = typeid(ComponentT).hash_code();
        if (_components.find(hashCode) != _components.end())
        {
            return dynamic_cast<ComponentT*>(_components[hashCode].get());
        }
        return nullptr;
    }
    
    void accept(NodeVisitor * visitor);
    void acceptReverse(NodeVisitor * visitor);
    
protected:
    std::vector<std::shared_ptr<Node>> _children;
    Node * _parent = nullptr;
    
    std::unordered_map<size_t, std::shared_ptr<Component>> _components;
    
    std::string _name;
    std::string _identifier;
    bool _disabled = false;
};

}
}
