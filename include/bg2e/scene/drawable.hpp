
#ifndef _bg2e_scene_drawable_hpp_
#define _bg2e_scene_drawable_hpp_

#include <bg2e/scene/component.hpp>
#include <bg2e/base/drawable_element.hpp>

namespace bg2e {
namespace scene {

	class Drawable : public Component {
	public:

		static Node * InstanceNode(Node * node);

		Drawable();
		Drawable(const std::string & name);

		virtual Component * clone();
		Drawable * instance(const std::string & newName);

		inline void setName(const std::string & newName) { _name = newName; }
		inline const std::string & name() const { return _name; }

		inline base::DrawableElementVector & drawableElements() { return _drawableElements; }
		inline const base::DrawableElementVector & drawableElements() const { return _drawableElements; }

		void addPolyList(base::PolyList * plist, base::Material * mat, const math::float4x4 & trx);
		void addPolyList(base::PolyList * plist, base::Material * mat);
		void addPolyList(base::PolyList * plist);
		void addPolyList(base::PolyList * plist, const math::float4x4 & trx);

		void removePolyList(base::PolyList * plist);

		size_t indexOf(base::PolyList * plist) const;

		base::DrawableElementVector::iterator element(size_t index);
		const base::DrawableElementVector::const_iterator element(size_t index) const;

		inline base::PolyList * polyList(size_t index) {
			base::DrawableElementVector::iterator result = element(index);
			if (result != _drawableElements.end()) {
				return (*result).polyList.getPtr();
			}
			return nullptr;
		}

		inline const base::PolyList * polyList(size_t index) const {
			base::DrawableElementVector::const_iterator result = element(index);
			if (result != _drawableElements.end()) {
				return (*result).polyList.getPtr();
			}
			return nullptr;
		}

		inline base::Material * material(size_t index) {
			base::DrawableElementVector::iterator result = element(index);
			if (result != _drawableElements.end()) {
				return (*result).material.getPtr();
			}
			return nullptr;
		}

		inline const base::Material * material(size_t index) const {
			base::DrawableElementVector::const_iterator result = element(index);
			if (result != _drawableElements.end()) {
				return (*result).material.getPtr();
			}
			return nullptr;
		}

		inline const math::float4x4 & transform(size_t index) const {
			base::DrawableElementVector::const_iterator result = element(index);
			if (result != _drawableElements.end()) {
				return (*result).transform;
			}
			return s_transformIdentity;
		}

		inline math::float4x4 & transform(size_t index) {
			base::DrawableElementVector::iterator result = element(index);
			if (result != _drawableElements.end()) {
				return (*result).transform;
			}
			return s_transformIdentity;
		}

		inline const base::DrawableElement * find(const std::string & name) const {
			for (auto & drwElem : _drawableElements) {
				if (drwElem.polyList->name() == name) {
					return &drwElem;
				}
			}
			return nullptr;
		}

		inline base::DrawableElement * find(const std::string & name) {
			for (auto & drwElem : _drawableElements) {
				if (drwElem.polyList->name() == name) {
					return &drwElem;
				}
			}
			return nullptr;
		}

		void draw(base::RenderQueue &, base::MatrixState * matrixState);

		virtual void deserialize(bg2e::db::json::Value *, const bg2e::base::path &);
		virtual bg2e::db::json::Value* serialize(const bg2e::base::path &);

	protected:
		virtual ~Drawable();

		base::DrawableElementVector _drawableElements;
		std::string _name;

		static math::float4x4 s_transformIdentity;
	};

}
}

#endif
