
#ifndef _BG2_SCENE_NODE_HPP_
#define _BG2_SCENE_NODE_HPP_

#include <string>
#include <vector>

#include <component.hpp>

namespace bg2scene {

    class Node {
    public:
        Node();
        virtual ~Node();
    
        std::string name;
        bool enabled;
        bool steady;

        std::vector<Node*> children;
        std::vector<Component*> components;
    };

}

#endif
