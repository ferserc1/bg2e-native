#ifndef _bg2e_base_light_hpp_
#define _bg2e_base_light_hpp_

#include <bg2e/base/referenced_pointer.hpp>
#include <bg2e/math/vector.hpp>
#include <bg2e/math/matrix.hpp>
#include <bgfx/bgfx.h>

#include <vector>

namespace bg2e {
namespace base {

	class Light : public ReferencedPointer {
	public:
		typedef std::vector<Light *> LightRegistry;

		static void ActivateLight(ptr<Light> & l) { ActivateLight(l.getPtr()); }
		static void DeactivateLight(ptr<Light> & l) { DeactivateLight(l.getPtr()); }
		static void ActivateLight(Light *);
		static void DeactivateLight(Light *);
		static const LightRegistry & ActiveLights();

		enum LightType {
			kDirectional = 4,
			kSpot = 1,
			kPoint = 5,
			kDisabled = 10
		};

		Light(bgfx::ViewId viewId);

		inline bgfx::ViewId viewId() const { return _viewId; }

		inline void setType(LightType t) { _type = t; }
		inline LightType type() const { return _type; }
		inline void setEnabled(bool e) { _enabled = e; }
		inline bool enabled() const { return _enabled; }
		inline void setPosition(const math::float3 & p) { _position = p; }
		inline const math::float3 & position() const { return _position; }
		inline void setDirection(const math::float3 & d, bool normalize = true) { _direction = d; if (normalize) _direction.normalize(); }
		inline const math::float3 & direction() const { return _direction; }
		inline void setDiffuse(const math::color & d) { _diffuse = d;}
		inline const math::color & diffuse() const { return _diffuse; }
		inline void setAttenuation(const math::float3 & a) { _attenuation = a; }
		inline const math::float3 & attenuation() const { return _attenuation; }
		inline void setConstantAttenuation(float a) { _attenuation[0] = a; }
		inline void setLinearAttenuationo(float a) { _attenuation[1] = a; }
		inline void setQuadraticAttenuation(float a) { _attenuation[2] = a; }
		inline float constantAttenuation() const { return _attenuation[0]; }
		inline float linearAttenuation() const { return _attenuation[1]; }
		inline float quadraticAttenuation() const { return _attenuation[2]; }
		inline void setIntensity(float i) { _intensity = i; }
		inline float intensity() const { return _intensity; }
		inline void setSpotCutoff(float s) { _spotCutoff = s; }
		inline float spotCutoff() const { return _spotCutoff; }
		inline void setSpotExponent(float e) { _spotExponent = e; }
		inline float spotExponent() const { return _spotExponent; }
		inline void shadowStrength(float s) { _shadowStrength = s; }
		inline float shadowStrength() const { return _shadowStrength; }
		inline void setCutoffDistance(float d) { _cutoffDistance = d; }
		inline float cutoffDistance() const { return _cutoffDistance; }
		inline void setShadowBias(float b) { _shadowBias = b; }
		inline float shadowBias() const { return _shadowBias; }
		inline void setCastShadows(bool c) { _castShadows = c; }
		inline bool castShadows() const { return _castShadows; }

	protected:
		virtual ~Light();

		bgfx::ViewId _viewId;

		LightType _type = kDirectional;
		bool _enabled = true;
		math::float3 _position = { 0.0f, 0.0f, 0.0f };
		math::float3 _direction = { 0.0f, 0.0f, -1.0f };
		math::color _diffuse = math::color(0xFFFFFFFF);
		math::float3 _attenuation = { 1.0f, 0.5f, 0.1f };
		float _intensity = 1.0f;
		float _spotCutoff = 20.0f;
		float _spotExponent = 30.0f;
		float _shadowStrength = 0.5f;
		float _cutoffDistance = -1.0f;
		float _shadowBias = 0.00002f;
		bool _castShadows = true;

		static LightRegistry s_lightRegistry;
	};

}
}

#endif
