#ifndef _bg2e_bgfx_tools_hpp_
#define _bg2e_bgfx_tools_hpp_

#include <bgfx/embedded_shader.h>

// Customized bgfx macros

// Embedded shader with Vulkan support disabled (the Vulkan shader compiler is buggy)
#define BG2E_EMBEDDED_SHADER(_name)                                                                \
			{                                                                                      \
				#_name,                                                                            \
				{                                                                                  \
					BGFX_EMBEDDED_SHADER_DX9BC(bgfx::RendererType::Direct3D9,  _name)              \
					BGFX_EMBEDDED_SHADER_DXBC (bgfx::RendererType::Direct3D11, _name)              \
					BGFX_EMBEDDED_SHADER_DXBC (bgfx::RendererType::Direct3D12, _name)              \
					BGFX_EMBEDDED_SHADER_PSSL (bgfx::RendererType::Gnm,        _name)              \
					BGFX_EMBEDDED_SHADER_METAL(bgfx::RendererType::Metal,      _name)              \
					BGFX_EMBEDDED_SHADER_NVN  (bgfx::RendererType::Nvn,        _name)              \
					BGFX_EMBEDDED_SHADER_ESSL (bgfx::RendererType::OpenGLES,   _name)              \
					BGFX_EMBEDDED_SHADER_GLSL (bgfx::RendererType::OpenGL,     _name)              \
					/* BGFX_EMBEDDED_SHADER_SPIRV(bgfx::RendererType::Vulkan,     _name) */              \
					{ bgfx::RendererType::Noop,  (const uint8_t*)"VSH\x5\x0\x0\x0\x0\x0\x0", 10 }, \
					{ bgfx::RendererType::Count, NULL, 0 }                                         \
				}                                                                                  \
			}

#define BG2E_EMBEDDED_SHADER_END()                         \
			{                                              \
				NULL,                                      \
				{                                          \
					{ bgfx::RendererType::Count, NULL, 0 } \
				}                                          \
			}

#endif
