#ifndef _bg2e_utils_load_hpp_
#define _bg2e_utils_load_hpp_

#include <bgfx/bgfx.h>
#include <bx/readerwriter.h>

namespace bg2e {
namespace utils {

    void* load(bx::FileReaderI* _reader, bx::AllocatorI* _allocator, const char* _filePath, uint32_t* _size);

    void unload(bx::AllocatorI* _allocator, void* _ptr);

    const bgfx::Memory* loadMem(bx::FileReaderI* _reader, const char* _filePath);

    void* loadMem(bx::FileReaderI* _reader, bx::AllocatorI* _allocator, const char* _filePath, uint32_t* _size);

    bgfx::ShaderHandle loadShader(bx::FileReaderI* _reader, const char* _name);

    bgfx::ProgramHandle loadProgram(bx::FileReaderI* _reader, const char* _vsName, const char* _fsName);
    
}
}

#endif

