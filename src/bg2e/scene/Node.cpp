//
//  Node.cpp

#include <bg2e/scene/Node.hpp>
#include <bg2e/utils/utils.hpp>

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

void Node::addChild(std::shared_ptr<Node> node)
{
    _children.push_back(node);
}

void Node::removeChild(std::shared_ptr<Node> node)
{
    // TODO: Implement this
}

const std::vector<std::shared_ptr<Node>>& Node::children() const
{
    return _children;
}

void Node::addComponent(std::shared_ptr<Component> comp)
{
    auto compPtr = comp.get();
    _components[typeid(*compPtr).name()] = comp;
}

void Node::removeComponent(std::shared_ptr<Component> comp)
{
    // TODO: Implement this
}

const std::unordered_map<std::string, std::shared_ptr<Component>>& Node::components() const
{
    return _components;
}

}
