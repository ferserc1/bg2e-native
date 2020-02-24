
#include <bg2render/poly_list.hpp>
#include <iostream>
#include <stdexcept>

namespace bg2render {

	void PolyList::configureVertexInput(bg2render::Pipeline* pipeline) {
		pipeline->vertexInputInfo().vertexBindingDescriptionCount = 1;
		pipeline->vertexInputInfo().pVertexBindingDescriptions = &PolyList::Vertex::getBindingDescription();
		pipeline->vertexInputInfo().vertexAttributeDescriptionCount = static_cast<uint32_t>(PolyList::Vertex::getAttributeDescriptions().size());
		pipeline->vertexInputInfo().pVertexAttributeDescriptions = PolyList::Vertex::getAttributeDescriptions().data();
	}

	VkVertexInputBindingDescription PolyList::s_bindingDescription = {
		0,	// binding
		sizeof(PolyList::Vertex),	// Stride
		VK_VERTEX_INPUT_RATE_VERTEX	// Input rate
	};
	bool PolyList::s_attributeInitialized = false;
	std::array<VkVertexInputAttributeDescription, 5> PolyList::s_attributeDescriptions;


	PolyList::PolyList()
	{
	}

	PolyList::~PolyList() {
		if (_instance) {
			_vertexBuffer = nullptr;
			_indexBuffer = nullptr;
		}
	}

	void PolyList::create(bg2render::vk::Instance* instance, VkCommandPool commandPool) {
		createTangents();

		_instance = instance;

		_vertexBuffer = std::make_unique<bg2render::VertexBuffer>(instance);
		_vertexBuffer->create<Vertex>(_vertices, commandPool);

		_indexBuffer = std::make_unique<bg2render::IndexBuffer>(instance);
		_indexBuffer->create<uint32_t>(_indices, commandPool);
	}

	void PolyList::bindBuffers(bg2render::vk::CommandBuffer* commandBuffer) {
		commandBuffer->bindVertexBuffer(0, 1, _vertexBuffer);
		commandBuffer->bindIndexBuffer(_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	}

	void PolyList::draw(bg2render::vk::CommandBuffer* commandBuffer) {
		commandBuffer->drawIndexed(_indices.size(), 1, 0, 0, 0);
	}

	void PolyList::createTangents() {
		using namespace bg2math;
		bool invalidTangents = false;
		if (_indices.size() % 3 == 0) {
			for (size_t i = 0; i < _indices.size(); i += 3) {
				auto& v0 = _vertices[_indices[i]];
				auto& v1 = _vertices[_indices[i + 1]];
				auto& v2 = _vertices[_indices[i + 2]];

				auto& p0 = v0.position;
				auto& p1 = v1.position;
				auto& p2 = v2.position;

				auto& t0 = v0.texCoord0;
				auto& t1 = v1.texCoord0;
				auto& t2 = v2.texCoord0;

				auto edge1 = p1 - p0;
				auto edge2 = p2 - p0;

				auto deltaU1 = t1[0] - t0[0];
				auto deltaV1 = t1[1] - t0[1];
				auto deltaU2 = t2[0] - t0[0];
				auto deltaV2 = t2[1] - t0[1];

				float den = deltaU1 * deltaV2 - deltaU2 * deltaV1;
				if (den == 0.0f) {
					v0.tangent = float3(0.0f, 0.0f, 1.0f);
					invalidTangents = true;
				}
				else {
					auto f = 1.0f / den;
					v0.tangent = float3(
						deltaV2 * edge1[0] - deltaV1 * edge2[0],
						f * (deltaV2 * edge1[1] - deltaV1 * edge2[1]),
						f * (deltaV2 * edge1[2] - deltaV1 * edge2[2]));
					v1.tangent = v0.tangent;
					v2.tangent = v0.tangent;
				}
			}
		}
		else {
			// Other draw modes: lines, line_strip, etc
			for (auto& v : _vertices) {
				v.tangent = float3(0.0f, 0.0f, 1.0f);
			}
		}

		if (invalidTangents) {
			std::cout << "WARN: Invalid tangents generated. Check polygon data." << std::endl;
		}
	}
}
