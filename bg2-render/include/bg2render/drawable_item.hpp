
#ifndef _bg2render_drawable_item_hpp_
#define _bg2render_drawable_item_hpp_

#include <bg2render/poly_list.hpp>
#include <bg2render/material.hpp>
#include <bg2math/vector.hpp>
#include <bg2math/matrix.hpp>
#include <bg2render/renderer.hpp>
#include <bg2render/drawable_descriptor.hpp>

namespace bg2render {
	class DrawableItem {
	public:
		struct UniformBufferObject {
			bg2math::float4x4 model;
			bg2math::float4x4 view;
			bg2math::float4x4 proj;
		};

		DrawableItem(const std::string& descriptorType, Renderer* rend);
		virtual ~DrawableItem();

		void create(PolyList* pl, Material* mat);
		void update(uint32_t frameIndex);
		void draw(bg2render::vk::CommandBuffer* commandBuffer, bg2render::Pipeline* pipeline, uint32_t frameIndex);

		inline const bg2render::vk::Buffer* uniformBuffer(uint32_t frameIndex) const { return _uniformBuffers[frameIndex].get(); }
		inline const bg2render::vk::DeviceMemory* uniformBufferMemory(uint32_t frameIndex) const { return _uniformBuffersMemory[frameIndex].get(); }
		inline const bg2render::Descriptor* descriptor() const { return _descriptor.get(); }

		inline const Material* material() const { return _material.get(); }
		inline Material* material() { return _material.get(); }
		inline const PolyList* polyList() const { return _polyList.get(); }
		inline PolyList* polyList() { return _polyList.get(); }

	protected:
		std::string _descriptorType;
		bg2render::Renderer* _renderer;

		std::shared_ptr<PolyList> _polyList;
		std::shared_ptr<Material> _material;
		std::shared_ptr<bg2render::Descriptor> _descriptor;
		std::shared_ptr<bg2render::vk::PipelineLayout> _pipelineLayout;

		std::vector<std::unique_ptr<bg2render::vk::Buffer>> _uniformBuffers;
		std::vector<std::unique_ptr<bg2render::vk::DeviceMemory>> _uniformBuffersMemory;

	};
}

#endif
