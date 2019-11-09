#ifndef _bg2e_math_matrix_strategy_hpp_
#define _bg2e_math_matrix_strategy_hpp_

#include <bg2e/math/matrix.hpp>

namespace bg2e {
namespace math {

	class MatrixStrategy {
	public:
		MatrixStrategy() :_target(nullptr) {}
		MatrixStrategy(float4x4 * target) :_target(target) {}

		inline void setTarget(float4x4 * target) { _target = target; }
		inline float4x4 * target() { return _target; }
		inline const float4x4 * target() const { return _target; }

		virtual void apply() = 0;

	protected:
		float4x4 * _target;
	};
}
}

#endif
