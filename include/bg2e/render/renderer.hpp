#ifndef _bg2e_render_renderer_hpp_
#define _bg2e_render_renderer_hpp_

#include <bg2e/base/referenced_pointer.hpp>

namespace bg2e {
namespace render {


    class Renderer : public base::ReferencedPointer {
    public:
        static Renderer * Create();

    protected:
        Renderer();
        virtual ~Renderer();
    };

}
}

#endif
