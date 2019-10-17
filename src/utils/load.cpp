
#include <bg2e/utils/load.hpp>

#include <stdexcept>
#include <fstream>

namespace bg2e {
namespace utils {

    void* load(bx::FileReaderI* _reader, bx::AllocatorI* _allocator, const char* _filePath, uint32_t* _size) {
        if (bx::open(_reader, _filePath) )
        {
            uint32_t size = (uint32_t)bx::getSize(_reader);
            void* data = BX_ALLOC(_allocator, size);
            bx::read(_reader, data, size);
            bx::close(_reader);
            if (NULL != _size)
            {
                *_size = size;
            }
            return data;
        }
        else
        {
            throw std::ifstream::failure(std::string("Failed to load file ") + _filePath);
        }

        if (NULL != _size)
        {
            *_size = 0;
        }

        return NULL;
    }

    void unload(bx::AllocatorI * _allocator, void* _ptr) {
        BX_FREE(_allocator, _ptr);
    }

    const bgfx::Memory* loadMem(bx::FileReaderI* _reader, const char* _filePath) {
        if (bx::open(_reader, _filePath) )
        {
            uint32_t size = (uint32_t)bx::getSize(_reader);
            const bgfx::Memory* mem = bgfx::alloc(size+1);
            bx::read(_reader, mem->data, size);
            bx::close(_reader);
            mem->data[mem->size-1] = '\0';
            return mem;
        }

        throw std::ifstream::failure(std::string("Failed to load file ") + _filePath);
        return NULL;
    }

    void* loadMem(bx::FileReaderI* _reader, bx::AllocatorI* _allocator, const char* _filePath, uint32_t* _size) {
        if (bx::open(_reader, _filePath) )
        {
            uint32_t size = (uint32_t)bx::getSize(_reader);
            void* data = BX_ALLOC(_allocator, size);
            bx::read(_reader, data, size);
            bx::close(_reader);

            if (NULL != _size)
            {
                *_size = size;
            }
            return data;
        }

        throw std::ifstream::failure(std::string("Failed to load file ") + _filePath);
        return NULL;
    }

    bgfx::ShaderHandle loadShader(bx::FileReaderI* _reader, const char* _name) {
        char filePath[512];

        const char* shaderPath = "???";

        switch (bgfx::getRendererType() )
        {
        case bgfx::RendererType::Noop:
        case bgfx::RendererType::Direct3D9:  shaderPath = "shaders/dx9/";   break;
        case bgfx::RendererType::Direct3D11:
        case bgfx::RendererType::Direct3D12: shaderPath = "shaders/dx11/";  break;
        case bgfx::RendererType::Gnm:        shaderPath = "shaders/pssl/";  break;
        case bgfx::RendererType::Metal:      shaderPath = "shaders/metal/"; break;
        case bgfx::RendererType::Nvn:        shaderPath = "shaders/nvn/";   break;
        case bgfx::RendererType::OpenGL:     shaderPath = "shaders/glsl/";  break;
        case bgfx::RendererType::OpenGLES:   shaderPath = "shaders/essl/";  break;
        case bgfx::RendererType::Vulkan:     shaderPath = "shaders/spirv/"; break;

        case bgfx::RendererType::Count:
            BX_CHECK(false, "You should not be here!");
            break;
        }

        bx::strCopy(filePath, BX_COUNTOF(filePath), shaderPath);
        bx::strCat(filePath, BX_COUNTOF(filePath), _name);
        bx::strCat(filePath, BX_COUNTOF(filePath), ".bin");

        bgfx::ShaderHandle handle = bgfx::createShader(loadMem(_reader, filePath) );
        bgfx::setName(handle, _name);

        return handle;
    }

    bgfx::ProgramHandle loadProgram(bx::FileReaderI* _reader, const char* _vsName, const char* _fsName) {
        bgfx::ShaderHandle vsh = loadShader(_reader, _vsName);
        bgfx::ShaderHandle fsh = BGFX_INVALID_HANDLE;
        if (NULL != _fsName)
        {
            fsh = loadShader(_reader, _fsName);
        }

        return bgfx::createProgram(vsh, fsh, true /* destroy shaders when program is destroyed */);
    }

}
}
