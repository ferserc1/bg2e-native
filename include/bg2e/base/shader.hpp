#ifndef _bg2e_base_shader_hpp_
#define _bg2e_base_shader_hpp_

#include <bg2e/base/referenced_pointer.hpp>
#include <bg2e/base/material.hpp>
#include <bg2e/base/poly_list.hpp>
#include <bg2e/math/matrix.hpp>

#include <bgfx/bgfx.h>

#include <unordered_map>

namespace bg2e {
namespace base {

	class Pipeline;

	class Shader : public ReferencedPointer {
	public:
		Shader();
		
		void create();

		inline bool isCreated() const { return _created; }

		virtual void bindUniforms(Pipeline*, PolyList * plist, Material* material, const math::float4x4 & modelMatrix) = 0;

		inline const bgfx::ProgramHandle & programHandle() const { return _program; }

	protected:
		virtual ~Shader();

		bool _created = false;
		bgfx::ProgramHandle _program = BGFX_INVALID_HANDLE;

		virtual bgfx::ProgramHandle loadProgram(bgfx::RendererType::Enum type) = 0;
	};
}
}

#endif
