#ifndef bg2e_base_polylist_hpp
#define bg2e_base_polylist_hpp

#include <bg2e/export.hpp>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <string>
#include <vector>


namespace bg2e {
namespace base {

typedef enum {
    BufferTypeVertex        = 1 << 0,
    BufferTypeNormal        = 1 << 1,
    BufferTypeTexCoord0     = 1 << 2,
    BufferTypeTexCoord1     = 1 << 3,
    BufferTypeTexCoord2     = 1 << 4,
    BufferTypeColor         = 1 << 5,
    BufferTypeTangent       = 1 << 6,
    BufferTypeIndex         = 1 << 7
} BufferType;

typedef enum {
    DrawModePoints          = 0,
    DrawModeTriangles,
    DrawModeTriangleFan,
    DrawModeTriangleStrip,
    DrawModeLines,
    DrawModeLineStrip
} DrawMode;

typedef enum {
    RenderLayerAuto  = 0,
    RenderLayer0     = 1 << 0,
    RenderLayer1     = 1 << 1,
    RenderLayer2     = 1 << 2,
    RenderLayer3     = 1 << 3,
    RenderLayer4     = 1 << 4,
    RenderLayer5     = 1 << 5,
    RenderLayer6     = 1 << 6,
    RenderLayer7     = 1 << 7,
    RenderLayer8     = 1 << 8,
    RenderLayer9     = 1 << 9,
    RenderLayer10    = 1 << 10,
    RenderLayer11    = 1 << 11,
    RenderLayer12    = 1 << 12,
    RenderLayer13    = 1 << 13,
    RenderLayer14    = 1 << 14,
    RenderLayer15    = 1 << 15,
    RenderLayer16    = 1 << 16,
    RenderLayer17    = 1 << 17,
    RenderLayer18    = 1 << 18,
    RenderLayer19    = 1 << 19,
    RenderLayer20    = 1 << 20,
    RenderLayer21    = 1 << 21,
    RenderLayer22    = 1 << 22,
    RenderLayer23    = 1 << 23,
    RenderLayer24    = 1 << 24,
    RenderLayer25    = 1 << 25,
    RenderLayer26    = 1 << 26,
    RenderLayer27    = 1 << 27,
    RenderLayer28    = 1 << 28,
    RenderLayer29    = 1 << 29,
    RenderLayer30    = 1 << 30,
    RenderLayer31    = 1 << 31
} RenderLayerFlag;

typedef uint32_t RenderLayers;
const RenderLayerFlag RenderLayerOpaqueDefault = RenderLayer0;
const RenderLayerFlag RenderLayerTransparentDefault = RenderLayer15;
const RenderLayers RenderLayerAll = 0xFFFFFFFF;



typedef enum {
    FrontFaceCW = 0,
    FrontFaceCCW
} PolyListFrontFace;

typedef enum {
    CullFaceFront = 0,
    CullFaceBack,
    CullFaceFrontAndBack
} PolyListCullFace;

class PolyList {
public:
    PolyList();
    
    const std::vector<float>& vertex() const { return _vertex; }
    const std::vector<float>& normal() const { return _normal; }
    const std::vector<float>& texCoord0() const { return _texCoord0; }
    const std::vector<float>& texCoord1() const { return _texCoord1; }
    const std::vector<float>& texCoord2() const { return _texCoord2; }
    const std::vector<float>& color() const { return _color; }
    
    const std::vector<uint32_t>& index() const { return _index; }
    
    const std::vector<float>& tangent();
    
    bool validTangents();

    void rebuildTangents();
    
protected:
    RenderLayers _renderLayers = RenderLayerAuto;
    
    DrawMode _drawMode = DrawModeTriangles;
    
    std::string _name = "";
    std::string _groupName = "";
    bool _visible = true;
    bool _visibleToShadows = true;
    
    PolyListCullFace _cullFace = CullFaceBack;
    PolyListFrontFace _frontFace = FrontFaceCCW;
    bool _enableCullFace = true;
    
    std::vector<float> _vertex;
    std::vector<float> _normal;
    std::vector<float> _texCoord0;
    std::vector<float> _texCoord1;
    std::vector<float> _texCoord2;
    std::vector<float> _color;
    
    std::vector<uint32_t> _index;
    
    std::vector<float> _tangent;
    bool _rebuildTangents = true;
};

}
}

#endif
