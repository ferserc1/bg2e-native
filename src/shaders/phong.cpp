
#include <bg2e/shaders/phong.hpp>

#include <bg2e/platform.hpp>
#include <bg2e/bgfx_tools.hpp>
#include <bg2e/base/light.hpp>
#include <bg2e/base/pipeline.hpp>

#include <bgfx/embedded_shader.h>

#if BG2E_PLATFORM_WINDOWS==1
#include "win64/shaders.h"
#elif BG2E_PLATFORM_OSX==1
#include "osx/shaders.h"
#elif BG2E_PLATFORM_LINUX==1
#include "linux/shaders.h"
#endif

namespace bg2e {
namespace shaders {

    const bgfx::EmbeddedShader s_embeddedPhongShader[] = {
        BG2E_EMBEDDED_SHADER(shaders::phong_vertex),
        BG2E_EMBEDDED_SHADER(shaders::phong_fragment),

        BG2E_EMBEDDED_SHADER_END()
    };

    Phong::Phong()
    {

    }

    Phong::~Phong() {

    }

	void Phong::bindFrameUniforms(base::Pipeline * pl) {
		base::Light* light = nullptr;

		for (auto* l : base::Light::ActiveLights()) {
			if (l->enabled() && pl->viewId()==l->viewId()) {
				light = l;
				break;
			}
		}

		if (light) {
			auto lightDir = light->direction();
			bgfx::setUniform(_uniforms.lightDirection, &lightDir);
		}
		else {
			math::float3 lightDir = math::float3(-0.8, 0.9, -0.5);
			lightDir.normalize();
			bgfx::setUniform(_uniforms.lightDirection, &lightDir);
		}
	}

    void Phong::bindUniforms(base::Pipeline *, base::PolyList * plist, base::Material * material, const math::float4x4 & modelMatrix) {
        math::float4x4 normalMatrix = modelMatrix;
        normalMatrix.invert().transpose();
        
        bgfx::setUniform(_uniforms.normalMatrix, normalMatrix.raw());

        bgfx::setTexture(0, _uniforms.diffuse, material->diffuse().texture->textureHandle());
        bgfx::setTexture(1, _uniforms.normal, material->normal().texture->textureHandle());
        bgfx::setTexture(2, _uniforms.ambientOcclussion, material->ambientOcclussion().texture->textureHandle());
        bgfx::setTexture(3, _uniforms.roughness, material->roughness().texture->textureHandle());
        bgfx::setUniform(_uniforms.fresnel, &material->fresnel());
    }

    bgfx::ProgramHandle Phong::loadProgram(bgfx::RendererType::Enum type) {
        _uniforms.normalMatrix = bgfx::createUniform("u_normalMatrix", bgfx::UniformType::Mat4);
        _uniforms.lightDirection = bgfx::createUniform("u_lightDirection", bgfx::UniformType::Vec4);

        _uniforms.diffuse = bgfx::createUniform("u_diffuse", bgfx::UniformType::Sampler);
        _uniforms.normal = bgfx::createUniform("u_normal", bgfx::UniformType::Sampler);
        _uniforms.ambientOcclussion = bgfx::createUniform("u_ambientOcclussion", bgfx::UniformType::Sampler);
        _uniforms.roughness = bgfx::createUniform("u_roughness", bgfx::UniformType::Sampler);
        _uniforms.fresnel = bgfx::createUniform("u_fresnel", bgfx::UniformType::Vec4);
        //_uniforms.diffuseUV = bgfx::createUniform("", bgfx::UniformType::);
        //_uniforms.normalUV = bgfx::createUniform("", bgfx::UniformType::);
        //_uniforms.ambientOcclussionUV = bgfx::createUniform("", bgfx::UniformType::);
        //_uniforms.roughnessUV = bgfx::createUniform("", bgfx::UniformType::);
        //_uniforms.fresnelUV = bgfx::createUniform("", bgfx::UniformType::);
        //_uniforms.roughnessChannel = bgfx::createUniform("", bgfx::UniformType::);
        //_uniforms.ambientOcclussionChannel = bgfx::createUniform("", bgfx::UniformType::);

        auto vsh = bgfx::createEmbeddedShader(s_embeddedPhongShader, type, "shaders::phong_vertex");
        auto fsh = bgfx::createEmbeddedShader(s_embeddedPhongShader, type, "shaders::phong_fragment");

        return bgfx::createProgram(vsh, fsh, true);
    }
}
}
