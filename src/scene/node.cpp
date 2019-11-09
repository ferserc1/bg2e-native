
#include <bg2e/scene/node.hpp>

namespace bg2e {
namespace scene {

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

	void Node::updateComponentVector() {
		_componentVector.clear();
		for (auto c : _componentMap) {
			_componentVector.push_back(c.second.getPtr());
		}
	}

}
}
