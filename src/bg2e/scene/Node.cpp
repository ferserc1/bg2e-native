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

void Node::addChild(Node * node)
{
    addChild(std::shared_ptr<Node>(node));
}

void Node::addChild(std::shared_ptr<Node> node)
{
    _children.push_back(node);
    node->_parent = this;
}

void Node::removeChild(std::shared_ptr<Node> node)
{
    // TODO: Implement this
}

const std::vector<std::shared_ptr<Node>>& Node::children() const
{
    return _children;
}

void Node::addComponent(Component * comp)
{
    addComponent(std::shared_ptr<Component>(comp));
}

void Node::addComponent(std::shared_ptr<Component> comp)
{
    _components[componentHash(comp.get())] = comp;
}

void Node::removeComponent(std::shared_ptr<Component> comp)
{
    // TODO: Implement this
}

const std::unordered_map<size_t, std::shared_ptr<Component>>& Node::components() const
{
    return _components;
}

void Node::accept(NodeVisitor * visitor)
{
    if (!visitor->ignoreDisabled() || !_disabled)
    {
        visitor->visit(this);
        for (auto child : _children)
        {
            child->accept(visitor);
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
    
}
