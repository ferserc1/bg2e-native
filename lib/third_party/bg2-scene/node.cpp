
#include <node.hpp>

namespace bg2scene {

    Node::Node() {}

    Node::~Node() {
        for (auto it = children.begin(); it != children.end(); ++it) {
            Node * child = *it;
            delete child;
        }
        children.clear();

        for (auto it = components.begin(); it != components.end(); ++it) {
            Component * cmp = *it;
            delete cmp;
        }
        components.clear();
    }

}

