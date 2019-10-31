
#ifndef _bg2e_utils_mesh_generators_hpp_
#define _bg2e_utils_mesh_generators_hpp_

#include <vector>

#include <bg2e/base/poly_list.hpp>

namespace bg2e {
namespace utils {

    extern void generateCube(float x, float y, float z, base::MeshData & result);
    inline void generateCube(float size, base::MeshData & result) { generateCube(size, size, size, result); }

}
}
#endif

