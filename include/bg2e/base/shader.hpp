#ifndef _bg2e_base_shader_hpp_
#define _bg2e_base_shader_hpp_

#include <bg2e/base/referenced_pointer.hpp>
#include <bg2e/base/material.hpp>
#include <bg2e/base/poly_list.hpp>

namespace bg2e {
namespace base {

	class Shader : public ReferencedPointer {
	public:
		Shader();
		
	protected:
		virtual ~Shader();
	};
}
}

#endif
