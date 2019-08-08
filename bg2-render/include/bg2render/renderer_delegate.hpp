
#ifndef _bg2render_renderer_delegate_hpp_
#define _bg2render_renderer_delegate_hpp_

#include <bg2render/pipeline.hpp>
#include <bg2render/vk_instance.hpp>
#include <bg2render/swap_chain.hpp>

namespace bg2render {

    class RendererDelegate {
    public:
		virtual Pipeline * configurePipeline(vk::Instance * instance, SwapChain * swapChain, const bg2math::int2 & frameSize) = 0;
    };
}

#endif
