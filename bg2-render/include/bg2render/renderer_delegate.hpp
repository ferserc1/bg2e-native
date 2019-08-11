
#ifndef _bg2render_renderer_delegate_hpp_
#define _bg2render_renderer_delegate_hpp_

#include <bg2render/pipeline.hpp>
#include <bg2render/vk_instance.hpp>
#include <bg2render/swap_chain.hpp>
#include <bg2render/vk_command_buffer.hpp>
#include <bg2math/vector.hpp>

namespace bg2render {

	class Renderer;
    class RendererDelegate {
		friend class Renderer;
    public:
		// Required renderer delegate methods:
		virtual Pipeline * configurePipeline(vk::Instance * instance, SwapChain * swapChain, const bg2math::int2 & frameSize) = 0;
		virtual void recordCommandBuffer(float delta, vk::CommandBuffer* cmdBuffer, Pipeline* pipeline, SwapChain * swapChain) = 0;

		virtual void initDone(vk::Instance *) {}

		virtual void beginRenderPass(vk::CommandBuffer* cmdBuffer, Pipeline* pipeline, VkFramebuffer framebuffer, SwapChain * swapChain);
		virtual void endRenderPass(vk::CommandBuffer * cmdBuffer);
		virtual void cleanup() {}

		inline void setClearColor(const bg2math::color& color) { _clearColor = color; }
		inline bg2math::color& clearColor() { return _clearColor; }
		inline const bg2math::color& clearColor() const { return _clearColor; }

		inline Renderer* renderer() { return _renderer; }
		inline const Renderer* renderer() const { return _renderer; }

	protected:
		bg2math::color _clearColor = bg2math::color(0.0f, 0.0f, 0.0f, 1.0f);
		
		Renderer* _renderer;
    };
}

#endif
