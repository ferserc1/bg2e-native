
#ifndef _bg2render_renderer_hpp_
#define _bg2render_renderer_hpp_

#include <bg2math/vector.hpp>
#include <bg2render/vk_instance.hpp>
#include <bg2render/pipeline.hpp>
#include <bg2render/swap_chain.hpp>
#include <bg2render/render_pass.hpp>
#include <bg2render/renderer_delegate.hpp>

#include <memory>

namespace bg2render {

	class Renderer {
	public:
		Renderer(vk::Instance* instance);
		virtual ~Renderer();

		inline void setDelegate(RendererDelegate* del) { _delegate = std::shared_ptr<RendererDelegate>(del); }

		void init(const bg2math::int2 & frameSize);

		void resize(const bg2math::int2 & frameSize);

		void update(float delta);

		void draw();

	protected:
		vk::Instance * _instance;

		std::shared_ptr<RendererDelegate> _delegate;

		// Swap chain
		std::shared_ptr<SwapChain> _swapChain;

		std::shared_ptr<Pipeline> _pipeline;

		// Commands
		VkCommandPool _commandPool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> _commandBuffers;

		// synchronization
		size_t _maxInFlightFrames = 0;
		size_t _currentFrame = 0;
		std::vector<VkSemaphore> _imageAvailableSemaphore;
		std::vector<VkSemaphore> _renderFinishedSemaphore;
		std::vector<VkFence> _inFlightFences;

		void configureRenderingObjects();
		void freeRenderingObjects();

		virtual void createPipeline(const bg2math::int2& frameSize);

	};

}

#endif
