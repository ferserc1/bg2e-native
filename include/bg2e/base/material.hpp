
#ifndef _bg2e_base_material_hpp_
#define _bg2e_base_material_hpp_


#include <bg2e/base/referenced_pointer.hpp>
#include <bg2e/base/texture.hpp>
#include <bg2e/math/vector.hpp>

namespace bg2e {
namespace base {

	class Material {
	public:

		struct TextureSet {
			ptr<Texture>	texture;
			math::float2	scale;
			uint16_t		uvSet;
			uint16_t		channel;
		};

		Material();

		inline const TextureSet& diffuse() const { return _diffuse; }
		inline const TextureSet& metallic() const { return _metallic; }
		inline const TextureSet& roughness() const { return _roughness; }
		inline const TextureSet& lightEmission() const { return _lightEmission; }
		inline const TextureSet& ambientOcclussion() const { return _ambientOcclussion; }
		inline const TextureSet& height() const { return _height; }
		inline const TextureSet& normal() const { return _normal; }

		inline const math::color & fresnel() const { return _fresnel; }
		inline bool isTransparent() const { return _isTransparent; }
		inline float alphaCutoff() const { return _alphaCutoff; }
		inline float heightIntensity() const { return _heightIntensity; }
		inline bool castShadows() const { return _castShadows; }
		inline bool cullFace() const { return _cullFace; }
		inline bool unlit() const { return _unlit; }


		void setDiffuse(const math::color& c);
		void setMetallic(float m);
		void setRoughness(float r);
		void setLightEmission(float e);
		void setAmbientOcclussion(float a);
		void setHeight(float h);
		void setNormal(const math::color& c);

		void setDiffuse(Texture * t);
		void setMetallic(Texture * t, uint16_t channel);
		void setRoughness(Texture * t, uint16_t channel);
		void setLightEmission(Texture * t, uint16_t channel);
		void setAmbientOcclussion(Texture * t, uint16_t channel);
		void setHeight(Texture * t, uint16_t channel);
		void setNormal(Texture * t);

		void setDiffuseUV(uint16_t uv) { _diffuse.uvSet = uv; }
		void setMetallicUV(uint16_t uv) { _metallic.uvSet = uv; }
		void setRoughnessUV(uint16_t uv) { _roughness.uvSet = uv; }
		void setLightEmissionUV(uint16_t uv) { _lightEmission.uvSet = uv; }
		void setAmbientOcclussionUV(uint16_t uv) { _ambientOcclussion.uvSet = uv; }
		void setHeightUV(uint16_t uv) { _height.uvSet = uv; }
		void setNormalUV(uint16_t uv) { _normal.uvSet = uv; }

		void setDiffuseScale(const math::float2& s) { _diffuse.scale = s; }
		void setMetallicScale(const math::float2& s) { _metallic.scale = s; }
		void setRoughnessScale(const math::float2& s) { _roughness.scale = s; }
		void setLightEmissionScale(const math::float2& s) { _lightEmission.scale = s; }
		void setAmbientOcclussionScale(const math::float2& s) { _ambientOcclussion.scale = s; }
		void setHeightScale(const math::float2& s) { _height.scale = s; }
		void setNormalScale(const math::float2& s) { _normal.scale = s; }

		inline void setFresnel(math::color& c) { _fresnel = c; }
		inline void setIsTransparent(bool t) { _isTransparent = t; }
		inline void setAlphaCutoff(float ac) { _alphaCutoff = ac; }
		inline void setCastShadows(bool c) { _castShadows = c; }
		inline void setCullFace(bool c) { _cullFace = c; }
		inline void setUnlit(bool ul) { _unlit = ul; }

	protected:
		virtual ~Material();

		TextureSet _diffuse;
		TextureSet _metallic;
		TextureSet _roughness;
		TextureSet _lightEmission;
		TextureSet _ambientOcclussion;
		TextureSet _height;
		TextureSet _normal;

		math::color _fresnel;
		bool _isTransparent;
		float _alphaCutoff;
		float _heightIntensity;
		bool _castShadows;
		bool _cullFace;
		bool _unlit;
	};

}
}

#endif

