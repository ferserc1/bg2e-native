#ifndef bg2e_base_material_hpp
#define bg2e_base_material_hpp

#include <bg2e/export.hpp>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace bg2e {
namespace base {

typedef enum
{
    MaterialTypePBR
} MaterialType;

class BG2E_EXPORT Material
{
public:
    Material();
    
protected:
    MaterialType type = MaterialTypePBR;
    
    glm::vec4 _diffuse;
    glm::vec2 _diffuseScale;
    
};

}
}
#endif
