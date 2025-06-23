
#include <bg2e.hpp>

class ClearScreenDelegate : public bg2e::render::RenderLoopDelegate,
	public bg2e::app::InputDelegate,
	public bg2e::ui::UserInterfaceDelegate
{
public:
	void init(bg2e::render::Engine * vulkan) override
	{
		using namespace bg2e::render::vulkan;
		RenderLoopDelegate::init(vulkan);
  
        _engine->descriptorSetAllocator().requirePoolSizeRatio(1, {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
        });
	}
 
    void executeComputeShader()
    {
        VkPipelineLayout computeLayout;
        VkPipeline computePipeline;
    
        // Resulting image
        _image = std::shared_ptr<bg2e::render::vulkan::Image>(bg2e::render::vulkan::Image::createAllocatedImage(
            _engine,
            VK_FORMAT_R8G8B8A8_UNORM, { 512, 512},
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT)
        );
        _engine->cleanupManager().push([&](VkDevice dev) {
            _image.reset();
        });
        
        bg2e::render::vulkan::factory::ComputePipeline plFactory(_engine);
        bg2e::render::vulkan::factory::PipelineLayout layoutFactory(_engine);
        bg2e::render::vulkan::factory::DescriptorSetLayout dsFactory;
        
        dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        auto descriptorSetLayout = dsFactory.build(_engine->device().handle(), VK_SHADER_STAGE_COMPUTE_BIT);
        
        layoutFactory.addDescriptorSetLayout(descriptorSetLayout);
        
        struct ComputePushConstant
        {
            uint32_t imageWidth;
            uint32_t imageHeight;
        };
        layoutFactory.addPushConstantRange(0, sizeof(ComputePushConstant), VK_SHADER_STAGE_COMPUTE_BIT);
        
        computeLayout = layoutFactory.build();
        
        plFactory.setShader("test.comp.spv");
        computePipeline = plFactory.build(computeLayout);
        
        
        // To execute a command shader independent of the rendering loop, we cannot
        // use the global descriptorSetAllocator; we have to create our own.
        auto descriptorSetAllocator = new bg2e::render::vulkan::DescriptorSetAllocator();
        descriptorSetAllocator->init(_engine);
        descriptorSetAllocator->initPool(1, {
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
        });
        
        auto descriptorSet = descriptorSetAllocator->allocate(descriptorSetLayout);
        descriptorSet->beginUpdate();
        descriptorSet->addImage(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, _image->imageView(), VK_IMAGE_LAYOUT_GENERAL);
        descriptorSet->endUpdate();
        
        // Execute one shoot command buffer
        _engine->command().immediateSubmit([&, computePipeline, computeLayout, descriptorSet](VkCommandBuffer cmd) {
            // Execute compute shader to generate the texture
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
            
            ComputePushConstant pushConstants {
                _image->extent().width,
                _image->extent().height
            };
            vkCmdPushConstants(cmd, computeLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstant), &pushConstants);
            
            VkDescriptorSet ds = descriptorSet->descriptorSet();
            vkCmdBindDescriptorSets(
                cmd,
                VK_PIPELINE_BIND_POINT_COMPUTE,
                computeLayout,
                0,
                1,
                &ds,
                0,
                nullptr
            );
            
            vkCmdDispatch(cmd, uint32_t(std::ceil(_image->extent().width / 16.0f)), uint32_t(std::ceil(_image->extent().height / 16.0f)), 1);
        });
        
        
        // Release the compute shader resources
        delete descriptorSetAllocator;
        
        vkDestroyPipeline(_engine->device().handle(), computePipeline, nullptr);
        vkDestroyPipelineLayout(_engine->device().handle(), computeLayout, nullptr);
        vkDestroyDescriptorSetLayout(_engine->device().handle(), descriptorSetLayout, nullptr);
        
    }
    
    void initScene() override
    {
        createImage(_engine->swapchain().extent());
  
        executeComputeShader();
        
        createPipeline();
    }

	void swapchainResized(VkExtent2D newExtent) override
	{
		_targetImage->cleanup();
		createImage(newExtent);
	}

	VkImageLayout render(
		VkCommandBuffer cmd,
		uint32_t currentFrame,
		const bg2e::render::vulkan::Image* colorImage,
		const bg2e::render::vulkan::Image* depthImage,
		bg2e::render::vulkan::FrameResources& frameResources
	) override {
		using namespace bg2e::render::vulkan;

		Image::cmdTransitionImage(
			cmd,
			_targetImage->handle(),
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL
		);

		float flash = std::abs(std::sin(currentFrame / 120.0f));
		VkClearColorValue clearValue{ { 0.0f, 0.0f, flash, 1.0f } };
		auto clearRange = Image::subresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
		vkCmdClearColorImage(
			cmd,
			_targetImage->handle(),
			VK_IMAGE_LAYOUT_GENERAL,
			&clearValue, 1, &clearRange
		);

		Image::cmdTransitionImage(
			cmd, _targetImage->handle(),
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);

		auto colorAttachment = Info::attachmentInfo(_targetImage->imageView(), nullptr);
		auto renderInfo = Info::renderingInfo(_targetImage->extent2D(), &colorAttachment, nullptr);
		cmdBeginRendering(cmd, &renderInfo);

		macros::cmdSetDefaultViewportAndScissor(cmd, _targetImage->extent2D());

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
        auto ds = _textureDS->descriptorSet();
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _layout, 0, 1, &ds, 0, nullptr);
		vkCmdDraw(cmd, 6, 1, 0, 0);

		bg2e::render::vulkan::cmdEndRendering(cmd);

		Image::cmdTransitionImage(
			cmd,
			_targetImage->handle(),
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
		);
		Image::cmdTransitionImage(
			cmd,
			colorImage->handle(),
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		);

		Image::cmdCopy(
			cmd,
			_targetImage->handle(), _targetImage->extent2D(),
			colorImage->handle(), colorImage->extent2D()
		);
  
		return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	}

	// ============ User Interface Delegate Functions =========
	void init(bg2e::render::Engine *, bg2e::ui::UserInterface*) override {
		_window.setTitle("ImGui Wrapper Demo");
		_window.options.noClose = true;
        _window.options.minWidth = 450;
        _window.options.minHeight = 100;
        _window.setPosition(0, 0);
        _window.setSize(45, 100);
	}

	void drawUI() override
	{
		using namespace bg2e::ui;

		//bg2e::ui::DemoWindow::draw();
		_window.draw([]() {
			BasicWidgets::text("Texture generated in compute shader");
		});
	}

protected:
	std::shared_ptr<bg2e::render::vulkan::Image> _targetImage;

	bg2e::ui::Window _window;
 
    VkPipelineLayout _layout;
    VkPipeline _pipeline;
    VkDescriptorSetLayout _textureDSLayout;
    std::unique_ptr<bg2e::render::vulkan::DescriptorSet> _textureDS;
    VkSampler _textureSampler;
    
    // Texture generated in a compute shader at initialization.
    // All the compute shader resources are created and released in the init()
    // function, after the image is generated
    std::shared_ptr<bg2e::render::vulkan::Image> _image;
    
    void createPipeline()
    {
        bg2e::render::vulkan::factory::GraphicsPipeline plFactory(_engine);
        bg2e::render::vulkan::factory::PipelineLayout layoutFactory(_engine);
        bg2e::render::vulkan::factory::DescriptorSetLayout dsFactory;
        bg2e::render::vulkan::factory::Sampler samplerFactory(_engine);
        
        plFactory.addShader("test.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        plFactory.addShader("test.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
        
        dsFactory.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        _textureDSLayout = dsFactory.build(_engine->device().handle(), VK_SHADER_STAGE_FRAGMENT_BIT);
        _textureSampler = samplerFactory.build();
        
        _textureDS = std::unique_ptr<bg2e::render::vulkan::DescriptorSet>(_engine->descriptorSetAllocator().allocate(_textureDSLayout));
        _textureDS->beginUpdate();
        _textureDS->addImage(
            0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            _image->imageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            _textureSampler
        );
        _textureDS->endUpdate();
        
        
        layoutFactory.addDescriptorSetLayout(_textureDSLayout);
        
        _layout = layoutFactory.build();
        
        plFactory.setColorAttachmentFormat(_targetImage->format());
        _pipeline = plFactory.build(_layout);
        
        _engine->cleanupManager().push([&](VkDevice dev) {
            vkDestroyPipeline(dev, _pipeline, nullptr);
            vkDestroyPipelineLayout(dev, _layout, nullptr);
            vkDestroyDescriptorSetLayout(dev, _textureDSLayout, nullptr);
            _textureDS.reset();
        });
    }

	void createImage(VkExtent2D extent)
	{
		using namespace bg2e::render::vulkan;
		auto engine = this->engine();
		_targetImage = std::shared_ptr<Image>(Image::createAllocatedImage(
			engine,
			VK_FORMAT_R16G16B16A16_SFLOAT,
			extent,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT
		));
	}

	void cleanup() override
	{
		_targetImage->cleanup();
	}
};

class MyApplication : public bg2e::app::Application {
public:
	void init(int argc, char** argv) override
	{
		auto delegate = std::shared_ptr<ClearScreenDelegate>(new ClearScreenDelegate());
		setRenderDelegate(delegate);
		setInputDelegate(delegate);
		setUiDelegate(delegate);
	}
};

int main(int argc, char ** argv)
{
    bg2e::app::MainLoop mainLoop;
	MyApplication app;
	app.init(argc, argv);
    return mainLoop.run(&app);
}
