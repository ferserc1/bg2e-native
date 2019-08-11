
#ifndef _bg2render_buffer_utils_hpp_
#define _bg2render_buffer_utils_hpp_

#include <bg2render/vk_instance.hpp>
#include <bg2render/vk_buffer.hpp>

namespace bg2render {

    class BufferUtils {
	public:
		static void CopyBuffer(vk::Instance * instance, VkCommandPool pool, vk::Buffer * srcBuffer, vk::Buffer * dstBuffer);
    };

}

#endif
