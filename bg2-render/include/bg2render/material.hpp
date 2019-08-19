#ifndef _bg2render_material_hpp_
#define _bg2render_material_hpp_

#include <bg2render/texture.hpp>

namespace bg2render{

	class Material {
	public:
		Material() {}

		inline void setDiffuse(bg2render::Texture* tex) { _diffuse = std::shared_ptr<bg2render::Texture>(tex); }
		inline const bg2render::Texture* diffuse() const { return _diffuse.get(); }

		inline void setNormalMap(bg2render::Texture* tex) { _normalMap = std::shared_ptr<bg2render::Texture>(tex); }
		inline const bg2render::Texture* normalMap() const { return _normalMap.get(); }

		inline void setMetallic(bg2render::Texture* tex) { _metallic = std::shared_ptr<Texture>(tex); }
		inline const bg2render::Texture* metallic() const { return _metallic.get(); }

		inline void setRoughness(bg2render::Texture* tex) { _roughness = std::shared_ptr<Texture>(tex); }
		inline const bg2render::Texture* roughness() const { return _roughness.get(); }

		inline void setEmission(bg2render::Texture* tex) { _emission = std::shared_ptr<Texture>(tex); }
		inline const bg2render::Texture* emission() const { return _emission.get(); }

		inline void setAmbientOcclusion(bg2render::Texture* tex) { _ambientOcclusion = std::shared_ptr<Texture>(tex); }
		inline const bg2render::Texture* ambientOcclusion() const { return _ambientOcclusion.get(); }

	protected:
		std::shared_ptr<bg2render::Texture> _diffuse;
		std::shared_ptr<bg2render::Texture> _normalMap;
		std::shared_ptr<bg2render::Texture> _roughness;
		std::shared_ptr<bg2render::Texture> _metallic;
		std::shared_ptr<bg2render::Texture> _emission;
		std::shared_ptr<bg2render::Texture> _ambientOcclusion;
	};
}

#endif
