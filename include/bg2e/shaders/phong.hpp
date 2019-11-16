
#ifndef _bg2e_shaders_phong_hpp_
#define _bg2e_shaders_phong_hpp_

#include <bg2e/base/shader.hpp>
#include <bg2e/base/material.hpp>
#include <bg2e/base/poly_list.hpp>

namespace bg2e {
namespace shaders {

    /*
     *  Basic debug phong shader generated from standard material properties, using 
     *  a single directional white light.
     *  - Diffuse, normal, ambient occlussion, UV sets and texture channel
     *    are equivalent in phong shader.
     *  - Roughness: The shininess value is generated as 1 / roughness.
     *  - Fresnel: Used as specular color.
     *  - The rest of the material properties are ignored.
	 *  - Only compatible with one light source (the first active and enabled light).
     */
    class Phong : public base::Shader {
    public:
        Phong();

		virtual void bindFrameUniforms(base::Pipeline *);
        virtual void bindUniforms(base::Pipeline *, base::PolyList * plist, base::Material * material, const math::float4x4 & modelMatrix, const math::float4x4 & inverseModelMatrix);

    protected:
        virtual ~Phong();

        virtual bgfx::ProgramHandle loadProgram(bgfx::RendererType::Enum type);

        struct Uniforms {
            bgfx::UniformHandle normalMatrix = BGFX_INVALID_HANDLE;
            // TODO: lighting uniforms
            bgfx::UniformHandle lightDirection = BGFX_INVALID_HANDLE;

            bgfx::UniformHandle diffuse = BGFX_INVALID_HANDLE;
            bgfx::UniformHandle normal = BGFX_INVALID_HANDLE;
            bgfx::UniformHandle ambientOcclussion = BGFX_INVALID_HANDLE;
            bgfx::UniformHandle roughness = BGFX_INVALID_HANDLE;
            bgfx::UniformHandle fresnel = BGFX_INVALID_HANDLE;
            bgfx::UniformHandle diffuseUV = BGFX_INVALID_HANDLE;
            bgfx::UniformHandle normalUV = BGFX_INVALID_HANDLE;
            bgfx::UniformHandle ambientOcclussionUV = BGFX_INVALID_HANDLE;
            bgfx::UniformHandle roughnessUV = BGFX_INVALID_HANDLE;
            bgfx::UniformHandle fresnelUV = BGFX_INVALID_HANDLE;
            bgfx::UniformHandle roughnessChannel = BGFX_INVALID_HANDLE;
            bgfx::UniformHandle ambientOcclussionChannel = BGFX_INVALID_HANDLE;
        } _uniforms;
    };
}
}


#endif
