
#include <bg2e/scene/transform.hpp>

#include <bg2e/scene/node.hpp>

namespace bg2e {
namespace scene {

	static void worldMatrix(Node * node, math::float4x4 & matrix) {
		if (node->parent()) {
			worldMatrix(node->parent(), matrix);
		}
		auto trx = node->transform();
		if (trx) {
			matrix = trx->matrix() * matrix;
		}
	}

	math::float4x4 Transform::WorldMatrix(Node * node) {
		auto result = math::float4x4::Identity();
		worldMatrix(node, result);
		return result;
	}

	Transform::Transform()
	{

	}

	Transform::~Transform() {

	}

	Component * Transform::clone() {
		return nullptr;
	}

	void Transform::willDraw(base::MatrixState * matrixState) {
		matrixState->model().push();
		matrixState->model().mult(_matrix);
	}

	void Transform::didDraw(base::RenderQueue &, base::MatrixState * matrixState) {
		matrixState->model().pop();
	}

	void Transform::deserialize(bg2e::db::json::Value *, const bg2e::base::path &) {

	}

	bg2e::db::json::Value * Transform::serialize(const bg2e::base::path &) {
		return nullptr;
	}

}
}
