
#ifndef _bg2render_offscreen_renderer_hpp_
#define _bg2render_offscreen_renderer_hpp_

#include <bg2render/vk_instance.hpp>
#include <bg2render/pipeline.hpp>
#include <bg2render/vk_image.hpp>
#include <bg2render/vk_device_memory.hpp>
#include <bg2render/vk_image_view.hpp>
#include <bg2render/vk_sampler.hpp>
#include <bg2render/render_pass.hpp>
#include <bg2math/vector.hpp>

namespace bg2render {

	class OffscreenRenderer {
	public:
		OffscreenRenderer(vk::Instance* instance);
		virtual ~OffscreenRenderer();

		void create(VkFormat format, const bg2math::uint2& size);

	protected:
		vk::Instance* _instance;

		bg2math::uint2 _size;
		VkFormat _colorFormat;
		VkFormat _depthFormat;

		// Temporal test pipeline
		std::unique_ptr<Pipeline> _pipeline;
		std::unique_ptr<vk::PipelineLayout> _pipelineLayout;

		// Color image
		std::unique_ptr<vk::Image> _colorImage;
		std::unique_ptr<vk::DeviceMemory> _colorImageMemory;
		std::unique_ptr<vk::ImageView> _colorImageView;
		std::unique_ptr<vk::Sampler> _colorSampler;
		
		// Depth image
		std::unique_ptr<vk::Image> _depthImage;
		std::unique_ptr<vk::DeviceMemory> _depthImageMemory;
		std::unique_ptr<vk::ImageView> _depthImageView;

		// Render pass
		std::unique_ptr<RenderPass> _renderPass;

		VkFramebuffer _framebuffer;
	};

}

#endif

