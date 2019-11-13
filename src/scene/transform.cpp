
#include <bg2e/scene/transform.hpp>

namespace bg2e {
namespace scene {

	Transform::Transform()
	{

	}

	Transform::~Transform() {

	}

	Component * Transform::clone() {
		return nullptr;
	}

	void Transform::deserialize(bg2e::db::json::Value *, const bg2e::base::path &) {

	}

	bg2e::db::json::Value * Transform::serialize(const bg2e::base::path &) {
		return nullptr;
	}

}
}
