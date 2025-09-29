//
//  MaterialEditor.hpp

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/MaterialBase.hpp>
#include <bg2e/ui/TextureWidgets.hpp>

#include <memory>

namespace bg2e {
namespace ui {

class BG2E_API MaterialEditor {
public:
    virtual ~MaterialEditor();

    void setEditMaterial(std::shared_ptr<render::MaterialBase>& mat);
    
    void clearMaterial();
    
    bool draw();

    void cleanup();
    
protected:
    std::shared_ptr<render::MaterialBase> _material;
    
    TextureWidgets _albedoWidget;
    TextureWidgets _normalWidget;
    TextureWidgets _metallicWidget;
    TextureWidgets _roughnessWidget;
    TextureWidgets _aoWidget;
    
    void initWidgets();
    void clearWidgets();
};

}
}
