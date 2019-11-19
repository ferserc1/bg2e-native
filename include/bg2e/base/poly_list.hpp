#ifndef _bg2e_base_poly_list_hpp_
#define _bg2e_base_poly_list_hpp_

#include <bg2e/base/referenced_pointer.hpp>
#include <bg2e/math/vector.hpp>

#include <bgfx/bgfx.h>

#include <vector>

namespace bg2e {
namespace base {

    enum CullFace {
        kCullFaceCW     = BGFX_STATE_CULL_CW,
        kCullFaceCCW    = BGFX_STATE_CULL_CCW
    };

    enum PolygonMode {
        kPolygonModeTriangleStrip   = BGFX_STATE_PT_TRISTRIP,
        kPolygonModeLines           = BGFX_STATE_PT_LINES,
        kPolygonModeLineStrip       = BGFX_STATE_PT_LINESTRIP,
        kPolygonModePoints          = BGFX_STATE_PT_POINTS,
        kPolygonModeTriangles       = 0
    };

	struct Vertex {
		math::float3 position;
		math::float3 normal;
		math::float2 uv0;
		math::float2 uv1;
		math::float3 tangent;

		static void init() {
			layout.begin()
				.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
				.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float, true)
				.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
				.add(bgfx::Attrib::TexCoord1, 2, bgfx::AttribType::Float)
				.add(bgfx::Attrib::Tangent, 3, bgfx::AttribType::Float, true)
				.end();
		}

		static bgfx::VertexLayout layout;
	};

    struct MeshData {
        std::vector<float> position;
        std::vector<float> normal;
        std::vector<float> texCoord0;
        std::vector<float> texCoord1;
        std::vector<float> tangent;
        std::vector<uint32_t> index;
    };

	class PolyList : public ReferencedPointer {
    protected:
        static void InitVertexLayouts();
        static bool s_vertexLayoutInitialized;
        
	public:
		PolyList();
		PolyList(const std::string & name);
		
		PolyList * clone(const std::string & name = "");

		inline const std::string & name() const { return _name; }
		inline void setName(const std::string & name) { _name = name; }
		inline const std::string & groupName() const { return _name; }
		inline void setGroupName(const std::string & groupName) { _groupName = groupName; }
		inline bool isVisible() const { return _visible; }
		inline bool isVisibleToShadows() const { return _visibleToShadows; }
		inline void setVisible(bool v) { _visible = v; }
		inline void setVisibleToShadows(bool v) { _visibleToShadows = v; }

        inline void setMeshData(const MeshData & md) {
            setPosition(md.position);
            setNormal(md.normal);
            setTexCoord0(md.texCoord0);
            setTexCoord1(md.texCoord1);
            setTangent(md.tangent);
            setIndex(md.index);
        }
        
        inline void setPosition(const std::vector<float> & p) { _position = p; }
        inline void setNormal(const std::vector<float> & n) { _normal = n; }
        inline void setTexCoord0(const std::vector<float> & t) { _texCoord0 = t; }
        inline void setTexCoord1(const std::vector<float> & t) { _texCoord1 = t; }
        inline void setTangent(const std::vector<float> & t) { _tangent = t; }
        inline void setIndex(const std::vector<uint32_t> & i) { _index = i; }
        
        inline const std::vector<float> & position() const { return _position; }
        inline const std::vector<float> & normal() const { return _normal; }
        inline const std::vector<float> & texCoord0() const { return _texCoord0; }
        inline const std::vector<float> & texCoord1() const { return _texCoord1; }
        inline const std::vector<float> & tangent() const { return _tangent; }
        inline const std::vector<uint32_t> & index() const { return _index; }
        
        inline void addPosition(float p) { _position.push_back(p); }
        inline void addPosition(float x, float y, float z) { _position.push_back(x); _position.push_back(y); _position.push_back(z); }
        inline void addPosition(const math::float3 & p) { _position.push_back(p.x()); _position.push_back(p.y()); _position.push_back(p.z()); }

		inline void addNormal(float n) { _normal.push_back(n); }
        inline void addNormal(float x, float y, float z) { _normal.push_back(x); _normal.push_back(y); _normal.push_back(z); }
        inline void addNormal(const math::float3 & n) { _normal.push_back(n.x()); _normal.push_back(n.y()); _normal.push_back(n.z()); }

		inline void addTexCoord0(float p) { _texCoord0.push_back(p); }
        inline void addTexCoord0(float u, float v) { _texCoord0.push_back(u); _texCoord0.push_back(v); }
        inline void addTexCoord0(const math::float3 & p) { _texCoord0.push_back(p.x()); _texCoord0.push_back(p.y()); }

		inline void addTexCoord1(float p) { _texCoord1.push_back(p); }
        inline void addTexCoord1(float u, float v) { _texCoord1.push_back(u); _texCoord1.push_back(v); }
        inline void addTexCoord1(const math::float3 & p) { _texCoord1.push_back(p.x()); _texCoord1.push_back(p.y()); }

		inline void addTangent(float p) { _tangent.push_back(p); }
        inline void addTangent(float x, float y, float z) { _tangent.push_back(x); _tangent.push_back(y); _tangent.push_back(z); }
        inline void addTangent(const math::float3 & p) { _tangent.push_back(p.x()); _tangent.push_back(p.y()); _tangent.push_back(p.z()); }
        
        inline void addIndex(uint32_t i) { _index.push_back(i); }
        inline void addTriangle(uint32_t v0, uint32_t v1, uint32_t v2) { _index.push_back(v0); _index.push_back(v1); _index.push_back(v2); }
        
        void buildTangents();
        
        void build();
        inline void build(const MeshData & meshData) {
            setMeshData(meshData);
            build();
        }
        
        inline const std::vector<Vertex> & vertexData() const { return _vertexData; }
        
        inline const bgfx::VertexBufferHandle & vertexBuffer() const { return _vertexBuffer; }
        inline const bgfx::IndexBufferHandle & indexBuffer() const { return _indexBuffer; }
        
        inline CullFace cullFace() const { return _cullFace; }
        inline void setCullFace(CullFace cf) { _cullFace = cf; }
        inline PolygonMode polygonMode() const { return _polygonMode; }
        inline void setPolygonMode(PolygonMode pm) { _polygonMode = pm; }
        
    protected:
		virtual ~PolyList();

		std::string _name;
		std::string _groupName;
		bool _visible = true;
		bool _visibleToShadows = true;

        std::vector<float> _position;
        std::vector<float> _normal;
        std::vector<float> _texCoord0;
        std::vector<float> _texCoord1;
        std::vector<float> _tangent;
        std::vector<uint32_t> _index;
        
        std::vector<Vertex> _vertexData;
        
        // bgfx data
        bgfx::VertexBufferHandle _vertexBuffer = BGFX_INVALID_HANDLE;
        bgfx::IndexBufferHandle _indexBuffer = BGFX_INVALID_HANDLE;
        
        CullFace _cullFace = kCullFaceCCW;
        PolygonMode _polygonMode = kPolygonModeTriangles;
	};

}
}

#endif
