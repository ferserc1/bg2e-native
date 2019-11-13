#ifndef _bg2e_base_drawable_element_hpp_
#define _bg2e_base_drawable_element_hpp_

#include <bg2e/base/poly_list.hpp>
#include <bg2e/base/material.hpp>
#include <bg2e/math/matrix.hpp>
#include <vector>

namespace bg2e {
namespace base {

	struct DrawableElement {
		ptr<base::PolyList> polyList;
		ptr<base::Material> material;
		math::float4x4 transform = math::float4x4::Identity();
		bool useTransform = false;

		inline void operator=(const DrawableElement & e) {
			polyList = e.polyList;
			material = e.material;
			transform = e.transform;
			useTransform = e.useTransform;
		}

		inline bool operator==(const DrawableElement & e) {
			return polyList.getPtr() == e.polyList.getPtr() &&
				material.getPtr() == e.material.getPtr() &&
				transform == e.transform &&
				useTransform == e.useTransform;
		}
	};

	typedef std::vector<DrawableElement> DrawableElementVector;

}
}

#endif
