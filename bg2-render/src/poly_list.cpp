
#include <bg2render/poly_list.hpp>
#include <iostream>

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
		std::cerr << "WARNING: PolyList::createTangents() not implemented." << std::endl;
	}
}
