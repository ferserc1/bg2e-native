
#ifndef _bg2e_scene_camera_hpp_
#define _bg2e_scene_camera_hpp_

#include <bg2e/scene/component.hpp>
#include <bg2e/base/camera.hpp>

namespace bg2e {
namespace scene {

    class Camera : public Component {
    public:
        Camera();
        
         
        inline base::Camera & camera() { return _camera; }
        inline const base::Camera & camera() const { return _camera; }
        
    protected:
        virtual ~Camera();
        
        base::Camera _camera;
    };

}
}

#endif
