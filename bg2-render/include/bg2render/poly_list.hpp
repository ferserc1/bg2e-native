
#ifndef _bg2render_poly_list_hpp_
#define _bg2render_poly_list_hpp_

#include <bg2render/pipeline.hpp>
#include <bg2render/vertex_buffer.hpp>
#include <bg2render/index_buffer.hpp>
#include <bg2render/vk_command_buffer.hpp>
#include <bg2math/vector.hpp>

#include <array>

namespace bg2render {
	class PolyList {
	public:
		struct Vertex {
			bg2math::float3 position;
			bg2math::float3 normal;
			bg2math::float2 texCoord0;
			bg2math::float2 texCoord1;
			bg2math::float3 tangent;

			static const VkVertexInputBindingDescription& getBindingDescription() {
				return s_bindingDescription;
			}

			static const std::array<VkVertexInputAttributeDescription, 5> & getAttributeDescriptions() {
				if (!s_attributeInitialized) {
					s_attributeDescriptions[0].binding = 0;
					s_attributeDescriptions[0].location = 0;
					s_attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
					s_attributeDescriptions[0].offset = offsetof(Vertex, position);

					s_attributeDescriptions[1].binding = 0;
					s_attributeDescriptions[1].location = 1;
					s_attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
					s_attributeDescriptions[1].offset = offsetof(Vertex, normal);

					s_attributeDescriptions[2].binding = 0;
					s_attributeDescriptions[2].location = 2;
					s_attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
					s_attributeDescriptions[2].offset = offsetof(Vertex, texCoord0);

					s_attributeDescriptions[3].binding = 0;
					s_attributeDescriptions[3].location = 3;
					s_attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
					s_attributeDescriptions[3].offset = offsetof(Vertex, texCoord1);

					s_attributeDescriptions[4].binding = 0;
					s_attributeDescriptions[4].location = 4;
					s_attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
					s_attributeDescriptions[4].offset = offsetof(Vertex, tangent);

					s_attributeInitialized = true;
				}

				return s_attributeDescriptions;
			}
		};

		static void configureVertexInput(bg2render::Pipeline* pipeline);

		inline void addVertex(
			const bg2math::float3& position,
			const bg2math::float3& normal,
			const bg2math::float2& texCoord0,
			const bg2math::float2& texCoord1
		) {
			_vertices.push_back({ position, normal, texCoord0, texCoord1, { 0.0f, 0.0f, 0.0f } });
		}

		inline void addVertex(
			const bg2math::float3& position,
			const bg2math::float3& normal,
			const bg2math::float2& texCoord0
		) {
			_vertices.push_back({ position, normal, texCoord0, texCoord0, { 0.0f, 0.0f, 0.0f } });
		}

		inline void addIndexedTriangle(uint32_t i0, uint32_t i1, uint32_t i2) {
			_indices.push_back(i0); _indices.push_back(i1); _indices.push_back(i2);
		}

		inline void addIndex(uint32_t i) { _indices.push_back(i); }

		inline const std::vector<Vertex>& vertices() const { return _vertices; }
		inline const std::vector<uint32_t>& indices() const { return _indices; }

		PolyList();
		virtual ~PolyList();

		void create(bg2render::vk::Instance* instance, VkCommandPool commandPool);
		void bindBuffers(bg2render::vk::CommandBuffer* commandBuffer);
		void draw(bg2render::vk::CommandBuffer* commandBuffer);

	protected:
		static VkVertexInputBindingDescription s_bindingDescription;
		static bool s_attributeInitialized;
		static std::array<VkVertexInputAttributeDescription, 5> s_attributeDescriptions;

		bg2render::vk::Instance* _instance = VK_NULL_HANDLE;

		// Vertex buffers
		std::unique_ptr<bg2render::VertexBuffer> _vertexBuffer;
		std::unique_ptr<bg2render::IndexBuffer> _indexBuffer;

		// Vertex and index data
		std::vector<Vertex> _vertices;
		std::vector<uint32_t> _indices;

		void createTangents();
	};

	
}

#endif
