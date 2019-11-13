
#include <bg2e/base/poly_list.hpp>

#include <unordered_map>

#include <stdexcept>

namespace bg2e {
namespace base {

	bgfx::VertexLayout Vertex::layout;

    void PolyList::InitVertexLayouts() {
        if (!s_vertexLayoutInitialized) {
            Vertex::init();
            s_vertexLayoutInitialized = true;
        }
    }

    bool PolyList::s_vertexLayoutInitialized = false;

	PolyList::PolyList()
	{
        InitVertexLayouts();
	}

	PolyList::PolyList(const std::string & name)
		:_name(name)
	{
		InitVertexLayouts();
	}

    PolyList::~PolyList() {
        bgfx::destroy(_vertexBuffer);
        bgfx::destroy(_indexBuffer);
    }

	PolyList * PolyList::clone(const std::string & name) {
		PolyList * newPlist = new PolyList();
		newPlist->_name = name.empty() ? name : _name;
		newPlist->_groupName = _groupName;

		newPlist->_position = _position;
		newPlist->_normal = _normal;
		newPlist->_texCoord0 = _texCoord0;
		newPlist->_texCoord1 = _texCoord1;
		newPlist->_tangent = _tangent;
		newPlist->_index = _index;

		newPlist->build();

		return newPlist;
	}

    void PolyList::buildTangents() {
        _tangent.clear();
        if (_texCoord0.size() == 0 || _position.size() == 0) {
            throw std::invalid_argument("Could not generate tangents: the vertex data is empty.");
        }
        
        std::unordered_map<int, math::float3> generatedIndexes;
        if (_index.size() % 3 == 0) {
            for (int i = 0; i<_index.size() - 2; i += 3) {
                int v0i = _index[i] * 3;
                int v1i = _index[i + 1] * 3;
                int v2i = _index[i + 2] * 3;
                
                int t0i = _index[i] * 2;
                int t1i = _index[i + 1] * 2;
                int t2i = _index[i + 2] * 2;
                
                math::float3 v0(_position[v0i], _position[v0i + 1], _position[v0i + 2]);
                math::float3 v1(_position[v1i], _position[v1i + 1], _position[v1i + 2]);
                math::float3 v2(_position[v2i], _position[v2i + 1], _position[v2i + 2]);
                
                math::float2 t0(_texCoord0[t0i], _texCoord0[t0i + 1]);
                math::float2 t1(_texCoord0[t1i], _texCoord0[t1i + 1]);
                math::float2 t2(_texCoord0[t2i], _texCoord0[t2i + 1]);
                
                math::float3 edge1 = v1 - v0;
                math::float3 edge2 = v2 - v0;
                
                float deltaU1 = t1.x() - t0.x();
                float deltaV1 = t1.y() - t0.y();
                float deltaU2 = t2.x() - t0.x();
                float deltaV2 = t2.y() - t0.y();
                
                float f = 1.0f / (deltaU1 * deltaV2 - deltaU2 * deltaV1);
                
                math::float3 tangent(f * (deltaV2 * edge1.x() - deltaV1 * edge2.x()),
                                f * (deltaV2 * edge1.y() - deltaV1 * edge2.y()),
                                f * (deltaV2 * edge1.z() - deltaV1 * edge2.z()));
                tangent.normalize();
                
                if (generatedIndexes.find(v0i) == generatedIndexes.end()) {
                    _tangent.push_back(tangent.x());
                    _tangent.push_back(tangent.y());
                    _tangent.push_back(tangent.z());
                    generatedIndexes[v0i] = tangent;
                }
                if (generatedIndexes.find(v1i) == generatedIndexes.end()) {
                    _tangent.push_back(tangent.x());
                    _tangent.push_back(tangent.y());
                    _tangent.push_back(tangent.z());
                    generatedIndexes[v1i] = tangent;
                }
                if (generatedIndexes.find(v2i) == generatedIndexes.end()) {
                    _tangent.push_back(tangent.x());
                    _tangent.push_back(tangent.y());
                    _tangent.push_back(tangent.z());
                    generatedIndexes[v2i] = tangent;
                }
            }
        }
        else {
            throw std::invalid_argument("Could not generate tangents: invalid vertex data. The tangents can only be generated for a triangle mesh.");
        }
    }

    void PolyList::build() {
        // Valid vertex data:
        //  - position, normal and texCoord0 are required
        //  - texCoord1 and tangent are optional
        if (((_position.size()  % 3) != 0 || _position.size()  == 0) ||
            ((_normal.size()    % 3) != 0 || _normal.size()    == 0) ||
            ((_texCoord0.size() % 2) != 0 || _texCoord0.size() == 0) ||
            (_texCoord1.size()  % 2) != 0 ||
            (_tangent.size()    % 3) != 0)
        {
            throw std::invalid_argument("Invalid vertex data");
        }
        
        if (_index.size()==0) {
            throw std::invalid_argument("Could not build polyList: invalid index data.");
        }
        
        if (_texCoord1.size() == 0) {
            _texCoord1 = _texCoord0;
        }
        if (_tangent.size() == 0) {
            buildTangents();
        }
        
        
        size_t it = 0;
        for (size_t i = 0; i < _position.size(); i+=3, it+=2) {
            float p[3] = { _position[i], _position[i + 1], _position[i + 2] };
            float n[3] = { _normal[i], _normal[i + 1], _normal[i + 2] };
            float t0[2] = { _texCoord0[it], _texCoord0[it + 1] };
            float t1[2] = { _texCoord1[it], _texCoord1[it + 1] };
            float tn[3] = { _tangent[i], _tangent[i + 1], _tangent[i + 2] };
            
            _vertexData.push_back({
                { p[0], p[1], p[2] },
                { n[0], n[1], n[2] },
                { t0[0], t0[1] },
                { t1[0], t1[1] },
                { tn[0], tn[1], tn[2] }
            });
        }
        
        auto vertexMem = bgfx::makeRef(_vertexData.data(), static_cast<uint32_t>(_vertexData.size() * sizeof(Vertex)));
        _vertexBuffer = bgfx::createVertexBuffer(vertexMem, Vertex::layout);
        
        auto indexMem = bgfx::makeRef(_index.data(), static_cast<uint32_t>(_index.size() * sizeof(uint32_t)));
        _indexBuffer = bgfx::createIndexBuffer(indexMem, BGFX_BUFFER_INDEX32);
    }

}
}
