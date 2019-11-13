//
//  phong_shader.hpp
//  cubes-example
//
//  Created by Fernando Serrano Carpena on 03/11/2019.
//  Copyright © 2019 Fernando Serrano Carpena. All rights reserved.
//

#ifndef phong_shader_hpp
#define phong_shader_hpp

#include <bg2e/base/shader.hpp>

class PhongShader : public bg2e::base::Shader {
public:
    PhongShader();

	virtual void bindFrameUniforms(bg2e::base::Pipeline*);
    virtual void bindUniforms(bg2e::base::Pipeline*, bg2e::base::PolyList* plist, bg2e::base::Material* material, bg2e::base::MatrixStack & modelMatrixStack);

protected:
    virtual ~PhongShader();

    virtual bgfx::ProgramHandle loadProgram(bgfx::RendererType::Enum type);

    bgfx::UniformHandle _lightPositionHandle = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle _normalMatHandle = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle _textureUniformHandle = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle _normalUniformHandle = BGFX_INVALID_HANDLE;
};

#endif /* phong_shader_hpp */
