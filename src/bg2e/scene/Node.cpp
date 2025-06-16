//
//  Node.cpp

#include <bg2e/scene/Node.hpp>
#include <bg2e/utils/utils.hpp>
#include <bg2e/scene/TransformVisitor.hpp>

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
    auto prevOwner = comp->_owner;
    comp->_owner = nullptr;
    if (prevOwner)
    {
        comp->removedFromNode(prevOwner);
    }

    auto plainPtr = comp.get();
    _components[BG2E_SCENE_COMP_CLASS_NAME(plainPtr)] = comp;
    comp->_owner = this;
    comp->addedToNode(this);
}

void Node::removeComponent(std::shared_ptr<Component> comp)
{
    // TODO: Implement this
}

const std::unordered_map<std::string, std::shared_ptr<Component>>& Node::components() const
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

Node * Node::sceneRoot()
{
    if (_parent == nullptr)
    {
        return this;
    }
    return _parent->sceneRoot();
}

}
