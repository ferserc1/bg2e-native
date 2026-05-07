/*
 *    business grade graphic engine (bg2 engine)
 *    Copyright (C) 2026  Fernando Serrano Carpena
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/Engine.hpp>

#include <string>
#include <memory>
#include <cstdint>

namespace bg2e::app {

/**
 * Configuration parameters for an offscreen application.
 *
 * This structure defines the properties of the render targets that will be
 * created and used during the lifetime of the application.
 */
struct OffscreenApplicationConfig
{
    /**
     * Width of the render target in pixels.
     */
    uint32_t width = 1920;

    /**
     * Height of the render target in pixels.
     */
    uint32_t height = 1080;

    /**
     * Indicates whether a color image should be created.
     *
     * If set to false, no color buffer will be explicitly requested. However,
     * a color image may still be created implicitly if createDepthImage is true.
     */
    bool createColorImage = true;

    /**
     * Indicates whether a depth image should be created.
     *
     * If this value is true, a color image will always be created as well,
     * even if createColorImage is set to false.
     */
    bool createDepthImage = true;

    /**
     * Format of the color image.
     *
     * This value defines the Vulkan format used when creating the offscreen
     * color buffer. It determines the channel layout, bit depth and encoding.
     *
     * The default value is VK_FORMAT_B8G8R8A8_UNORM.
     */
    VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
};

class OffscreenApplicationDelegate
{
public:
    /**
     * Called before any initialization is performed.
     *
     * This method allows the delegate to process command line arguments and
     * initialize the output configuration structure.
     *
     * @param argc Number of command line arguments.
     * @param argv Command line argument values.
     * @param outConfig Configuration object to initialize.
     */
    virtual void initConfig(
        [[maybe_unused]] int argc, [[maybe_unused]] char ** argv,
        [[maybe_unused]] OffscreenApplicationConfig & outConfig
    ) {}

    /**
     * Called when the rendering engine has been initialized.
     *
     * The colorImage and depthImage parameters may be nullptr depending on the
     * configuration provided in initConfig().
     *
     * @param engine Initialized rendering engine instance.
     * @param colorImage Offscreen color image, or nullptr if no color image was created.
     * @param depthImage Offscreen depth image, or nullptr if no depth image was created.
     */
    virtual void init(
        [[maybe_unused]] bg2e::render::Engine* engine,
        [[maybe_unused]] std::shared_ptr<render::vulkan::Image> colorImage,
        [[maybe_unused]] std::shared_ptr<render::vulkan::Image> depthImage
    ) {}

    /**
     * Called when the frame resources descriptor set allocator is about to be initialized.
     *
     * Offscreen applications do not use a swapchain or real-time frame rendering,
     * but a FrameResources object is still used to keep the API consistent with
     * the rest of the high-level rendering system.
     *
     * @param allocator Descriptor set allocator used by frame resources.
     */
    virtual void initFrameResources([[maybe_unused]] render::vulkan::DescriptorSetAllocator * allocator) {}

    /**
     * Called after the entire rendering engine and frame resource descriptor sets
     * have been fully initialized.
     */
    virtual void initScene() {}

    /**
     * Called once immediately after initScene() to maintain compatibility with
     * high-level APIs.
     *
     * Offscreen applications do not support resizing. The render targets remain
     * fixed for the entire lifetime of the application.
     *
     * @param width Render target width in pixels.
     * @param height Render target height in pixels.
     */
    virtual void resize(
        [[maybe_unused]] uint32_t width,
        [[maybe_unused]] uint32_t height
    ) {}

    /**
     * Called at the beginning of each frame.
     *
     * Intended for updating animations or time-dependent logic. The delta parameter
     * represents the elapsed time since the previous frame.
     *
     * @param delta Elapsed time since the previous frame, in seconds.
     * @param frameIndex Index of the frame being processed.
     * @param frameResources Frame resources object used for this frame.
     */
    virtual void frame(
        [[maybe_unused]] float delta,
        [[maybe_unused]] uint32_t frameIndex,
        [[maybe_unused]] render::vulkan::FrameResources& frameResources
    ) {}

    /**
     * Called to render the frame.
     *
     * The images used for rendering are the ones provided during init() and do not
     * change during execution.
     *
     * @param cmd Command buffer used to record rendering commands.
     * @param frameIndex Index of the frame being rendered.
     * @param frameResources Frame resources object used for this frame.
     * @param colorImageLayout The layout of the color image.
     * @param outFinalColorImageLayout The output layout of the color image at the end of the render function.
     * @return true to continue rendering more frames, or false to stop.
     */
    virtual bool render(
        VkCommandBuffer cmd,
        uint32_t frameIndex,
        render::vulkan::FrameResources& frameResources,
        VkImageLayout colorImageLayout,
        VkImageLayout &outFinalColorImageLayout
    ) = 0;

    /**
     * Called after a frame has been fully rendered.
     *
     * This is invoked after waiting for the command queue fence, so all rendering
     * operations are complete and the images are safe to access.
     *
     * At this point, the color image layout is VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
     * so it can be copied to a staging buffer.
     *
     * @param frameIndex Index of the frame that has just been rendered.
     * @param elapsedMs Time spent rendering the frame, in milliseconds.
     * @param colorImageLayout Layout of the color image at this point.
     */
    virtual void didRenderFrame(
        [[maybe_unused]] uint32_t frameIndex,
        [[maybe_unused]] double elapsedMs,
        [[maybe_unused]] VkImageLayout colorImageLayout
    ) {}

    /**
     * Called to manually release any resources created by the delegate.
     *
     * Prefer using Engine cleanupManager() or managed pointers whenever possible.
     * This method should only be used when those mechanisms are not sufficient.
     */
    virtual void cleanup() {}

    float delta() const { return _delta; }
    void setDelta(float delta) { _delta = delta; }

protected:
    float _delta = 0.0f;
};

class BG2E_API OffscreenApplication {
public:

    void init(
        int argc, char ** argv,
        const std::string& appId,
        std::shared_ptr<OffscreenApplicationDelegate> delegate
    );

    int run();

protected:
    void cleanup();

    OffscreenApplicationConfig _config;
    std::string _appId;

    std::shared_ptr<OffscreenApplicationDelegate> _delegate;

    std::unique_ptr<render::Engine> _engine;

    std::shared_ptr<render::vulkan::Image> _colorImage;
    std::shared_ptr<render::vulkan::Image> _depthImage;
};

}
