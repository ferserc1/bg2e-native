
#ifndef _bg2e_scene_camera_hpp_
#define _bg2e_scene_camera_hpp_

#include <bg2e/scene/component.hpp>
#include <bg2e/base/camera.hpp>

namespace bg2e {
namespace scene {

    class Camera : public Component {
    public:
        Camera();
        
		virtual Component* clone();
         
        inline base::Camera & camera() { return _camera; }
        inline const base::Camera & camera() const { return _camera; }
        
		virtual void deserialize(bg2e::db::json::Value*, const bg2e::base::path&);
		virtual bg2e::db::json::Value* serialize(const bg2e::base::path&);

    protected:
        virtual ~Camera();
        
        base::Camera _camera;
    };

}
}

#endif
