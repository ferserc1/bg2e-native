//
//  MaterialEditor.hpp

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/render/MaterialBase.hpp>
#include <bg2e/ui/TextureWidgets.hpp>

#include <memory>
#include <vector>
#include <functional>

namespace bg2e {
namespace ui {

class BG2E_API MaterialEditor {
public:
    virtual ~MaterialEditor();

    void setEditMaterial(std::shared_ptr<render::MaterialBase>& mat);
    inline std::shared_ptr<render::MaterialBase> editMaterial() { return _material; }
    inline const std::shared_ptr<render::MaterialBase> editMaterial() const { return _material; }
    
    void addEditMaterial(std::shared_ptr<render::MaterialBase>& mat);
    
    void clearMaterial();
    
    bool draw();

    void cleanup();

    inline void onChanged(std::function<void()> cb) { _onChangedFunction = cb; }

protected:
    std::shared_ptr<render::MaterialBase> _material;
    std::vector<std::shared_ptr<render::MaterialBase>> _editMaterialList;
    
    TextureWidgets _albedoWidget;
    TextureWidgets _normalWidget;
    TextureWidgets _metallicWidget;
    TextureWidgets _roughnessWidget;
    TextureWidgets _aoWidget;

    std::function<void()> _onChangedFunction;
    
    void initWidgets();
    void clearWidgets();

    inline void notifyOnChange()
    {
        if (_onChangedFunction)
        {
            _onChangedFunction();
        }
    }
};

}
}
