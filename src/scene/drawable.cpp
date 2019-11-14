
#include <bg2e/scene/drawable.hpp>
#include <bg2e/scene/node.hpp>

namespace bg2e {
namespace scene {

	math::float4x4 Drawable::s_transformIdentity = math::float4x4::Identity();

	Node * Drawable::InstanceNode(Node * node) {
		ptr<Node> newNode = new Node(node->name().empty() ? "" : "copy of " + node->name());
		newNode->setEnabled(node->isEnabled());
		for (auto comp : node->components()) {
			Component * newComponent = nullptr;
			Drawable * drw = dynamic_cast<Drawable*>(comp);
			if (drw) {
				newComponent = drw->instance(drw->name().empty() ? "" : "copy of " + drw->name());
			}
			else {
				newComponent = comp->clone();
			}
			newNode->addComponent(newComponent);
		}
		return newNode.release();
	}

	Drawable::Drawable()
	{

	}

	Drawable::Drawable(const std::string & name)
		:_name(name)
	{

	}

	Drawable::~Drawable() {

	}

	Component * Drawable::clone() {
		ptr<Drawable> newInstance = new Drawable(name());
		for (auto & elem : _drawableElements) {
			newInstance->addPolyList(elem.polyList->clone(),
				elem.material->clone(),
				elem.transform);
		}

		return newInstance.release();
	}

	Drawable * Drawable::instance(const std::string & newName) {
		std::string name = newName;
		if (name.empty()) {
			name = "Instance of " + _name;
		}
		ptr<Drawable> newInstance = new Drawable(name);
		for (auto & elem : _drawableElements) {
			newInstance->addPolyList(elem.polyList.getPtr(),
				elem.material->clone(),
				elem.transform);
		}

		return newInstance.release();
	}

	void Drawable::addPolyList(base::PolyList * plist, base::Material * mat, const math::float4x4 & trx) {
		_drawableElements.push_back({ plist, mat, trx, true });
	}

	void Drawable::addPolyList(base::PolyList * plist, base::Material * mat) {
		_drawableElements.push_back({ plist, mat, math::float4x4::Identity(), false });
	}

	void Drawable::addPolyList(base::PolyList * plist) {
		_drawableElements.push_back({ plist, new base::Material(), math::float4x4::Identity(), false });
	}

	void Drawable::addPolyList(base::PolyList * plist, const math::float4x4 & trx) {
		_drawableElements.push_back({ plist, new base::Material(), trx, true });
	}

	void Drawable::removePolyList(base::PolyList * plist) {
		auto it = std::begin(_drawableElements);
		for (auto item : _drawableElements) {
			if (item.polyList.getPtr() == plist) {
				break;
			}
			++it;
		}
		if (it != _drawableElements.end()) {
			_drawableElements.erase(it);
		}
	}

	size_t Drawable::indexOf(base::PolyList * plist) const {
		size_t index = 0;
		for (auto item : _drawableElements) {
			if (item.polyList.getPtr() == plist) {
				break;
			}
			++index;
		}
		return 0;
	}

	base::DrawableElementVector::iterator Drawable::element(size_t index) {
		if (index < _drawableElements.size()) {
			return _drawableElements.begin() + index;
		}
		return _drawableElements.end();
	}

	const base::DrawableElementVector::const_iterator Drawable::element(size_t index) const {
		if (index < _drawableElements.size()) {
			return _drawableElements.cbegin() + index;
		}
		return _drawableElements.cend();
	}

	void Drawable::draw(base::RenderQueue & renderQueue, base::Pipeline * pipeline) {
		for (auto item : drawableElements()) {
			auto trx = pipeline->model().matrix();
			if (item.polyList->isVisible()) {
				renderQueue.addPolyList(
					item.polyList.getPtr(),
					item.material.getPtr(),
					pipeline->model().matrix());
			}
		}
	}

	void Drawable::deserialize(bg2e::db::json::Value *, const bg2e::base::path &) {
		// TODO: implement drawable load plugin
	}

	bg2e::db::json::Value * Drawable::serialize(const bg2e::base::path &) {
		// TODO: implement drawable write plugin
		return nullptr;
	}

}
}
