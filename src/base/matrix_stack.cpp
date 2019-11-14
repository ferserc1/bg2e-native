
#include <bg2e/base/matrix_stack.hpp>

#include <stdexcept>

namespace bg2e {
namespace base {

	MatrixStack::MatrixStack()
	{
		_matrixStack.push_back(math::float4x4::Identity());
		_inverseMatrix = math::float4x4::Identity();
		_updateInverse = false;
	}

	void MatrixStack::push() {
		_matrixStack.push_back(_matrixStack.back());
		_updateInverse = true;
	}

	void MatrixStack::pop() {
		if (_matrixStack.size() > 1) {
			_matrixStack.pop_back();
			_updateInverse = true;
		}
		else {
			throw std::runtime_error("Matrix stack push/pop mismatch: the matrix stack size is zero");
		}
	}

	void MatrixStack::load(const math::float4x4 & m) {
		_matrixStack.back() = m;
		_updateInverse = true;
	}

	const math::float4x4 & MatrixStack::matrix() const {
		return _matrixStack.back();
	}

	math::float4x4 & MatrixStack::matrix() {
		_updateInverse = true;
		return _matrixStack.back();
	}

	const math::float4x4 & MatrixStack::inverseMatrix() {
		if (_updateInverse) {
			_inverseMatrix = _matrixStack.back();
			_inverseMatrix.invert();
		}
		return _inverseMatrix;
	}
	
	void MatrixStack::beginFrame() {
		_matrixStack.clear();
		_matrixStack.push_back(math::float4x4::Identity());
		_inverseMatrix = math::float4x4::Identity();
		_updateInverse = false;
	}

	MatrixStack & MatrixStack::identity() {
		load(math::float4x4::Identity());
		return *this;
	}

	MatrixStack & MatrixStack::translate(float x, float y, float z) {
		matrix().translate(x, y, z);
		return *this;
	}

	MatrixStack & MatrixStack::translate(const math::float3 & trx) {
		matrix().translate(trx);
		return *this;
	}

	MatrixStack & MatrixStack::rotate(float alpha, float x, float y, float z) {
		matrix().rotate(alpha, x, y, z);
		return *this;
	}

	MatrixStack & MatrixStack::rotate(float alpha, const math::float3 & axis) {
		matrix().rotate(alpha, axis.x(), axis.y(), axis.z());
		return *this;
	}

	MatrixStack & MatrixStack::rotateX(float alpha) {
		matrix().rotate(alpha, 1.0f, 0.0f, 0.0f);
		return *this;
	}

	MatrixStack & MatrixStack::rotateY(float alpha) {
		matrix().rotate(alpha, 0.0f, 1.0f, 0.0f);
		return *this;
	}

	MatrixStack & MatrixStack::rotateZ(float alpha) {
		matrix().rotate(alpha, 0.0f, 0.0f, 1.0f);
		return *this;
	}

	MatrixStack & MatrixStack::scale(float x, float y, float z) {
		matrix().scale(x, y, z);
		return *this;
	}

	MatrixStack & MatrixStack::scale(float s) {
		matrix().scale(s, s, s);
		return *this;
	}

	MatrixStack & MatrixStack::scale(const math::float3 & s) {
		matrix().scale(s);
		return *this;
	}

	MatrixStack & MatrixStack::mult(const math::float4x4 & other) {
		matrix() = matrix() * other;
		return *this;
	}

	MatrixStack & MatrixStack::perspective(float fovy, float aspect, float nearPlane, float farPlane) {
		matrix().perspective(fovy, aspect, nearPlane, farPlane);
		return *this;
	}

	MatrixStack & MatrixStack::frustum(float left, float right, float bottom, float top, float nearPlane, float farPlane) {
		matrix().frustum(left, right, bottom, top, nearPlane, farPlane);
		return *this;
	}

	MatrixStack & MatrixStack::ortho(float left, float right, float bottom, float top, float nearPlane, float farPlane) {
		matrix().ortho(left, right, bottom, top, nearPlane, farPlane);
		return *this;
	}

	MatrixStack & MatrixStack::lookAt(const math::float3 & origin, const math::float3 & target, const math::float3 & up) {
		matrix().lookAt(origin, target, up);
		return *this;
	}

}
}
