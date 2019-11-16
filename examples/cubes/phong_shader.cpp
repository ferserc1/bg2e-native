//
//  phong_shader.cpp
//  cubes-example
//
//  Created by Fernando Serrano Carpena on 03/11/2019.
//  Copyright © 2019 Fernando Serrano Carpena. All rights reserved.
//

#include "phong_shader.hpp"

#include <bg2e/platform.hpp>

#include <bg2e/bgfx_tools.hpp>

#include <bgfx/embedded_shader.h>

#if BG2E_PLATFORM_WINDOWS==1
#include "win64/example_shaders.h"
#elif BG2E_PLATFORM_OSX==1
#include "osx/example_shaders.h"
#elif BG2E_PLATFORM_LINUX==1
#include "linux/example_shaders.h"
#endif

const bgfx::EmbeddedShader s_phongShadersBasic[] = {
   BG2E_EMBEDDED_SHADER(shaders::phong_vertex),
   BG2E_EMBEDDED_SHADER(shaders::phong_fragment),

   BG2E_EMBEDDED_SHADER_END()
};

PhongShader::PhongShader()
{
    
}

PhongShader::~PhongShader() {
    
}

void PhongShader::bindFrameUniforms(bg2e::base::Pipeline*) {
	bgfx::setUniform(_lightPositionHandle, &bg2e::math::float4(2.0f, 2.0f, -5.0f, 0.0f));
}

void PhongShader::bindUniforms(bg2e::base::Pipeline*, bg2e::base::PolyList* plist, bg2e::base::Material * material, const bg2e::math::float4x4 & modelMatrix, const bg2e::math::float4x4 & inverseModelMatrix) {
	auto normalMatrix = inverseModelMatrix;
    normalMatrix.transpose();

    bgfx::setUniform(_normalMatHandle, normalMatrix.raw());
    bgfx::setTexture(0, _textureUniformHandle, material->diffuse().texture->textureHandle());
    bgfx::setTexture(1, _normalUniformHandle, material->normal().texture->textureHandle());
}



bgfx::ProgramHandle PhongShader::loadProgram(bgfx::RendererType::Enum type) {
    _lightPositionHandle = bgfx::createUniform("lightPosition", bgfx::UniformType::Vec4);
    _normalMatHandle = bgfx::createUniform("u_normal", bgfx::UniformType::Mat4);
    _textureUniformHandle = bgfx::createUniform("s_diffuseTexture", bgfx::UniformType::Sampler);
    _normalUniformHandle = bgfx::createUniform("s_normalTexture", bgfx::UniformType::Sampler);

    bgfx::ShaderHandle vsh = bgfx::createEmbeddedShader(s_phongShadersBasic, type, "shaders::phong_vertex");
    bgfx::ShaderHandle fsh = bgfx::createEmbeddedShader(s_phongShadersBasic, type, "shaders::phong_fragment");

    return bgfx::createProgram(vsh, fsh, true);
}

