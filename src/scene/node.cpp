
#include <bg2e/scene/node.hpp>

#include <bg2e/scene/camera.hpp>
#include <bg2e/scene/drawable.hpp>
#include <bg2e/scene/transform.hpp>

#include <stdexcept>

namespace bg2e {
namespace scene {

	std::vector<Node::RemoveNodeData> Node::s_removeNodeVector;
	std::vector<Node::AddNodeData> Node::s_addNodeVector;
	std::vector<Node::AddComponentData> Node::s_addComponentVector;

    Node::Node()
        :_name()
    {

    }

    Node::Node(const std::string & name)
        :_name(name)
    {

    }

    Node::Node(const char * name)
        :_name(name)
    {

    }

    Node::~Node() {

    }

	bool Node::addChild(Node* child) {
		if (child && !isAncestorOf(child) && child != this) {
			ptr<Node> node = child;
			if (node->_parent) {
				node->_parent->removeChild(node.getPtr());
			}
			_children.push_back(node.getPtr());
			node->_parent = this;
			return true;
		}
		return false;
	}

	bool Node::removeChild(Node* child) {
		if (child) {
			NodeVector::iterator it = findChildIterator(child);
			if (it != _children.end()) {
				(*it)->_parent = nullptr;
				_children.erase(it);
				return true;
			}
		}
		return false;
	}

	void Node::clearChildren()  {
		for (auto child : _children) {
			child->_parent = nullptr;
		}
		_children.clear();
	}

	bool Node::findChild(const Node* node) {
		auto it = findChildIterator(node);
		return it != _children.end() ? (*it).getPtr() : nullptr;
	}

	NodeVector::iterator Node::findChildIterator(const Node* node) {
		return std::find(_children.begin(), _children.end(), node);
	}

	bool Node::isAncestorOf(const Node* ancient) const {
		return IsAncestor(this, ancient);
	}

	bool Node::IsAncestor(const Node* node, const Node* antient) {
		if (!node || !antient) {
			return false;
		}
		else if (node->_parent == antient) {
			return true;
		}
		else {
			return IsAncestor(node->_parent, antient);
		}
	}

	void Node::accept(NodeVisitor* visitor) {
		if (visitor && isEnabled()) {
			visitor->visit(this);
			for (auto c : _children) {
				c->accept(visitor);
			}
			visitor->didVisit(this);
		}
	}

	void Node::acceptReverse(NodeVisitor* visitor) {
		if (visitor && isEnabled()) {
			if (_parent) {
				_parent->acceptReverse(visitor);
			}
			visitor->visit(this);
		}
	}

	void Node::RemoveFromScene(Node* node, SceneChangeFunction callback) {
		if (!node) {
			throw  std::invalid_argument("Node::RemoveFromScene(): null node specified.");
		}
		s_removeNodeVector.push_back(Node::RemoveNodeData{ node, callback });
	}

	void Node::AddToScene(Node* node, Node* parent, SceneChangeFunction callback) {
		if (!node) {
			throw std::invalid_argument("Node::AddToScene(): null node specified.");
		}
		if (!parent) {
			throw std::invalid_argument("Node::AddToScene(): null parent node specified.");
		}
		s_addNodeVector.push_back(Node::AddNodeData{ parent, node, callback });
	}

	void Node::AddComponent(Component* comp, Node* node, SceneChangeFunction callback) {
		if (!comp) {
			throw std::invalid_argument("Node::AddComponent(): null component specified.");
		}
		if (!node) {
			throw std::invalid_argument("Node::AddComponent(): null node specified.");
		}
		s_addComponentVector.push_back(Node::AddComponentData{ comp, node, callback });
	}

	void Node::ApplySceneChanges() {
		for (auto n : s_removeNodeVector) {
			Node * parent = n.node->parent();
			if (parent) {
				parent->removeChild(n.node.getPtr());
				if (n.didRemoved) {
					n.didRemoved(n.node.getPtr());
				}
			}
		}

		for (auto n : s_addNodeVector) {
			n.parent->addChild(n.child.getPtr());
			if (n.didAdded) {
				n.didAdded(n.child.getPtr());
			}
		}

		for (auto n : s_addComponentVector) {
			n.node->addComponent(n.component.getPtr());
			if (n.didAdded) {
				n.didAdded(n.node.getPtr());
			}
		}

		s_removeNodeVector.clear();
		s_addNodeVector.clear();
		s_addComponentVector.clear();
	}

	void Node::updateComponentVector() {
		_componentVector.clear();
		for (auto c : _componentMap) {
			_componentVector.push_back(c.second.getPtr());
		}
	}

	Camera * Node::camera() {
		return component<Camera>();
	}

	Transform * Node::transform() {
		return component<Transform>();
	}

	Drawable * Node::drawable() {
		return component<Drawable>();
	}

}
}
