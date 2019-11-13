#ifndef _bg2e_base_matrix_stack_hpp_
#define _bg2e_base_matrix_stack_hpp_

#include <bg2e/math/matrix.hpp>

#include <vector>

namespace bg2e {
namespace base {
	
	typedef std::vector<math::float4x4> MatrixStackContainer;

	class MatrixStack {
	public:
		MatrixStack();

		void push();
		void pull();
		void load(const math::float4x4 & m);
		const math::float4x4 & matrix() const;
		math::float4x4 & matrix();

		const math::float4x4 & inverseMatrix();

		void beginFrame();

		MatrixStack & loadIdentity();
		MatrixStack & translate(float x, float y, float z);
		MatrixStack & translate(const math::float3 & trx);
		MatrixStack & rotate(float alpha, float x, float y, float z);
		MatrixStack & rotate(float alpha, const math::float3 & axis);
		MatrixStack & rotateX(float alpha);
		MatrixStack & rotateY(float alpha);
		MatrixStack & rotateZ(float alpha);
		MatrixStack & scale(float x, float y, float z);
		MatrixStack & scale(float s);
		MatrixStack & scale(const math::float3 & s);
		MatrixStack & mult(const math::float4x4 & other);
		MatrixStack & perspective(float fovy, float aspect, float nearPlane, float farPlane);
		MatrixStack & frustum(float left, float right, float bottom, float top, float nearPlane, float farPlane);
		MatrixStack & ortho(float left, float right, float bottom, float top, float nearPlane, float farPlane);
		MatrixStack & lookAt(const math::float3 & origin, const math::float3 & target, const math::float3 & up);
		

	protected:
		MatrixStackContainer _matrixStack;
		bool _updateInverse;
		math::float4x4 _inverseMatrix;
	};

}
}

#endif
