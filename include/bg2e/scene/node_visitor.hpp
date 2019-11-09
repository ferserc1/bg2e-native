#ifndef _bg2e_scene_node_visitor_hpp_
#define _bg2e_scene_node_visitor_hpp_

#include <bg2e/base/referenced_pointer.hpp>

namespace bg2e {
namespace scene {

	class Node;
	class NodeVisitor : public base::ReferencedPointer {
	public:
		virtual void visit(Node * node);
		virtual void didVisit(Node * node);

	protected:
		virtual ~NodeVisitor();
	};

}
}

#endif
