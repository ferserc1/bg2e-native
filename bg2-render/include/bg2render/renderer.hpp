
#ifndef _bg2render_renderer_hpp_
#define _bg2render_renderer_hpp_

#include <bg2math/vector.hpp>
#include <bg2render/vk_instance.hpp>
#include <bg2render/pipeline.hpp>
#include <bg2render/swap_chain.hpp>
#include <bg2render/render_pass.hpp>
#include <bg2render/vk_command_buffer.hpp>
#include <bg2render/renderer_delegate.hpp>

#include <memory>

namespace bg2render {

	class Renderer {
	public:
		Renderer(vk::Instance* instance);
		virtual ~Renderer();

		inline void setDelegate(RendererDelegate* del) { _delegate = std::shared_ptr<RendererDelegate>(del); _delegate->_renderer = this; }

		void init(const bg2math::int2 & frameSize);

		void resize(const bg2math::int2 & frameSize);

		void update(float delta);

		void draw();

		inline vk::Instance* instance() const { return _instance; }
		inline VkCommandPool commandPool() const { return _commandPool; }
		inline SwapChain* swapChain() { return _swapChain.get(); }
		inline const SwapChain* swapChain() const { return _swapChain.get(); }
		inline vk::Image* depthImage() { return _depthImage.get(); }
		inline const vk::Image* depthImage() const { return _depthImage.get(); }
		inline vk::DeviceMemory* depthImageMemory() { return _depthImageMemory.get(); }
		inline const vk::DeviceMemory* depthImageMemory() const { return _depthImageMemory.get(); }
		inline vk::ImageView* depthImageView() { return _depthImageView.get(); }
		inline const vk::ImageView* depthImageView() const { return _depthImageView.get(); }
		inline Pipeline* pipeline() { return _pipeline.get(); }
		inline const Pipeline* pipeline() const { return _pipeline.get(); }

		inline uint32_t simultaneousFrames() const { return static_cast<uint32_t>(_swapChain->images().size()); }

	protected:
		vk::Instance * _instance;

		std::shared_ptr<RendererDelegate> _delegate;

		// Swap chain
		std::shared_ptr<SwapChain> _swapChain;

		std::shared_ptr<Pipeline> _pipeline;

		// Depth images
		std::shared_ptr<vk::Image> _depthImage;
		std::shared_ptr<vk::DeviceMemory> _depthImageMemory;
		std::shared_ptr<vk::ImageView> _depthImageView;

		// Commands
		VkCommandPool _commandPool = VK_NULL_HANDLE;
		std::vector<std::shared_ptr<vk::CommandBuffer>> _commandBuffers;

		// synchronization
		size_t _maxInFlightFrames = 0;
		size_t _currentFrame = 0;
		std::vector<VkSemaphore> _imageAvailableSemaphore;
		std::vector<VkSemaphore> _renderFinishedSemaphore;
		std::vector<VkFence> _inFlightFences;

		void createCommandPool();
		void configureRenderingObjects();
		void freeCommandPool();
		void freeRenderingObjects();
		void createDepthResources(const bg2math::int2& frameSize);

		virtual void createPipeline(const bg2math::int2& frameSize);

	};

}

#endif
