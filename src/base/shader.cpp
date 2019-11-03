
#include <bg2e/base/shader.hpp>

#include <bg2e/base/pipeline.hpp>

namespace bg2e {
namespace base {

	Shader::Shader()
	{

	}

	Shader::~Shader() {
		bgfx::destroy(_program);
	}

	void Shader::create() {
		bgfx::RendererType::Enum type = bgfx::getRendererType();
		_program = loadProgram(type);
		_created = true;
	}

}
}
