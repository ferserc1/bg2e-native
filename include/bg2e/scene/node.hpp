
#ifndef _bg2e_scene_node_hpp_
#define _bg2e_scene_node_hpp_

#include <bg2e/base/referenced_pointer.hpp>
#include <bg2e/scene/component.hpp>

#include <string>

namespace bg2e {
namespace scene {

    class Node : public base::ReferencedPointer {
    public:
        Node();
        Node(const std::string & name);
        Node(const char * name);

		inline void setSteady(bool s) { _steady = s; }
		inline bool isSteady() const { return _steady; }
		inline void setEnabled(bool e) { _enabled = e; }
		inline bool isEnabled() const { return _enabled; }

		void addComponent(Component* c) {
			if (c) {
				ptr<Component> preventDelete = c;

				if (c->_node != nullptr) {
					Node * parent = c->_node;
					parent->removeComponent(c);
					c->removedFromNode(parent);
				}

				if (_componentMap.find(hash(c)) != _componentMap.end() &&
					_componentMap[hash(c)].valid())
				{
					_componentMap[hash(c)]->removedFromNode(this);
					_componentMap[hash(c)]->_node = nullptr;
				}

				_componentMap[hash(c)] = c;
				c->_node = this;
				c->addedToNode(this);
				updateComponentVector();
			}
		}

		void removeComponent(Component* c) {
			auto it = _componentMap.find(hash(c));
			if (it != _componentMap.end()) {
				ptr<Component> preventDelete = c;
				c->_node = nullptr;
				_componentMap.erase(it);
				c->removedFromNode(this);
				updateComponentVector();
			}
		}

		template <class T>
		T * component() {
			return dynamic_cast<T*>(_componentMap[typeid(T).hash_code()]).getPtr();
		}

		inline const ComponentVectorWeak & components() const { return _componentVector; }
		

    protected:
        virtual ~Node();

        std::string _name;
        bool _steady;
        bool _enabled;

		// the ComponentMap stores strong references to components and is used for direct access.
		// ComponentVector stores weak references and is used for sequential iteration.
		ComponentMap _componentMap;
		ComponentVectorWeak _componentVector;

		inline size_t hash(Component * component) const { return typeid(*component).hash_code(); }

		void updateComponentVector();
    };

}
}

#endif
