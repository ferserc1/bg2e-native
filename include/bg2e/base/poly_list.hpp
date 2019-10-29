#ifndef _bg2e_base_poly_list_hpp_
#define _bg2e_base_poly_list_hpp_

#include <bg2e/math/vector.hpp>

#include <bgfx/bgfx.h>

namespace bg2e {
namespace base {

	struct Vertex {
		math::float3 position;
		math::float3 normal;
		math::float2 uv0;
		math::float2 uv1;
		math::float3 tangent;

		static void init() {
			layout.begin()
				.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
				.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float, true)
				.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
				.add(bgfx::Attrib::TexCoord1, 2, bgfx::AttribType::Float)
				.add(bgfx::Attrib::Tangent, 3, bgfx::AttribType::Float, true)
				.end();
		}

		static bgfx::VertexLayout layout;
	};

	class PolyList {
	public:
		PolyList();


	};

}
}

#endif
