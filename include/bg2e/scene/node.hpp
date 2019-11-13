
#ifndef _bg2e_scene_node_hpp_
#define _bg2e_scene_node_hpp_

#include <bg2e/base/referenced_pointer.hpp>
#include <bg2e/scene/component.hpp>
#include <bg2e/scene/node_visitor.hpp>

#include <string>

namespace bg2e {
namespace scene {

	class Node;
	typedef std::vector<ptr<Node>> NodeVector;

	class Camera;
	class Transform;
	class Drawable;

    class Node : public base::ReferencedPointer {
    public:
		typedef std::function<void(Node*)> SceneChangeFunction;

        Node();
        Node(const std::string & name);
        Node(const char * name);

		inline void setSteady(bool s) { _steady = s; }
		inline bool isSteady() const { return _steady; }
		inline void setEnabled(bool e) { _enabled = e; }
		inline bool isEnabled() const { return _enabled; }

		inline void setName(const std::string & name) { _name = name; }
		inline const std::string & name() const { return _name; }

		// Begin of component manipulation functions
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
            return dynamic_cast<T*>(_componentMap[typeid(T).hash_code()].getPtr());
        }

		inline const ComponentVectorWeak & components() const { return _componentVector; }

		// End of component manipulation functions

		// Begin of child node manipulation functions
		bool addChild(Node * child);
		bool removeChild(Node * child);
		void clearChildren();

		inline NodeVector & children() { return _children; }
		inline const NodeVector & children() const { return _children; }

		inline Node * parent() { return _parent; }
		inline const Node * parent() const { return _parent; }
		
		bool findChild(const Node * node);
		NodeVector::iterator findChildIterator(const Node * node);
		bool isAncestorOf(const Node * ancient) const;
		static bool IsAncestor(const Node * node, const Node * antient);
		inline Node * sceneRoot() { return _parent ? _parent->sceneRoot() : this; }
		inline const Node * sceneRoot() const { return _parent ? _parent->sceneRoot() : this; }
		// End of child node manipulation functions

		// Node visitor functions
		virtual void accept(NodeVisitor * visitor);
		virtual void acceptReverse(NodeVisitor * visitor);

		/// To add or remove nodes inside a life cycle function, use the static
		/// node manipulation functions. The node will be removed asynchronously when is
		/// safe to do it
		static void RemoveFromScene(Node *, SceneChangeFunction didRemoved = nullptr);
		static void AddToScene(Node * node, Node * parent, SceneChangeFunction didAdded = nullptr);
		static void AddComponent(Component * comp, Node * node, SceneChangeFunction didAdded = nullptr);

		// This function is called to apply the changes made by the static scene manipulation functions
		// It must be called rigth after the frame() function
		void ApplySceneChanges();

		// Direct access to the most common components
		Camera * camera();
		Transform * transform();
		Drawable * drawable();
		

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

		NodeVector _children;
		Node * _parent;

		struct RemoveNodeData {
			ptr<Node> node;
			SceneChangeFunction didRemoved;
		};

		struct AddNodeData {
			ptr<Node> parent;
			ptr<Node> child;
			SceneChangeFunction didAdded;
		};

		struct AddComponentData {
			ptr<Component> component;
			ptr<Node> node;
			SceneChangeFunction didAdded;
		};

		static std::vector<RemoveNodeData> s_removeNodeVector;
		static std::vector<AddNodeData> s_addNodeVector;
		static std::vector<AddComponentData> s_addComponentVector;
    };

}
}

#endif
