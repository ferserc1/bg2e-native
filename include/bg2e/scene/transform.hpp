
#ifndef _bg2e_scene_transform_hpp_
#define _bg2e_scene_transform_hpp_

#include <bg2e/scene/component.hpp>

namespace bg2e {
namespace scene {

	class Transform : public Component {
	public:
		Transform();

		virtual Component * clone();

		virtual void deserialize(bg2e::db::json::Value *, const bg2e::base::path &);
		virtual bg2e::db::json::Value* serialize(const bg2e::base::path &);

	protected:
		virtual ~Transform();

	};

}
}

#endif
