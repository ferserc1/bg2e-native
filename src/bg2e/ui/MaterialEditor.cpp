//
//  MaterialEditor.cpp

#include <bg2e/ui/MaterialEditor.hpp>
#include <bg2e/ui/BasicWidgets.hpp>
#include <bg2e/ui/Input.hpp>
#include <bg2e/render/vulkan/common.hpp>
#include <bg2e/render/vulkan/Image.hpp>
#include <bg2e/app/FileDialog.hpp>

#include "imgui.h"
#include "imgui_impl_vulkan.h"

namespace bg2e::ui {

MaterialEditor::~MaterialEditor()
{

}

void MaterialEditor::setEditMaterial(std::shared_ptr<render::MaterialBase>& mat)
{
    _editMaterialList.clear();
    clearWidgets();
    _material = mat;
    _editMaterialList.push_back(mat);
    initWidgets();
}

void MaterialEditor::clearMaterial()
{
    _material.reset();
    _editMaterialList.clear();
    clearWidgets();
}

void MaterialEditor::addEditMaterial(std::shared_ptr<render::MaterialBase>& mat)
{
    if (_material.get())
    {
        _editMaterialList.push_back(mat);
    }
    else
    {
        setEditMaterial(mat);
    }
}
    
bool MaterialEditor::draw()
{
    if (_material.get() && BasicWidgets::collapsingHeader(_material->materialAttributes().name() + "'s Material Attributes"))
    {
        std::vector<std::string> uvOptions = { "Set 0", "Set 1" };
        BasicWidgets::separator("Albedo");
        auto albedoColor = _material->materialAttributes().albedo();
        auto albedoScale = _material->materialAttributes().albedoScale();
        auto albedoUVSet = _material->materialAttributes().albedoUVSet();
        if (Input::colorPicker("Color##albedo", albedoColor))
        {
            for (auto mat : _editMaterialList)
            {
                mat->materialAttributes().setAlbedo(albedoColor);
            }
        }
        _albedoWidget.selectTexture("##albedo", [&](base::Texture* tex) {
            auto ptrTex = std::shared_ptr<base::Texture>(tex);
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setAlbedoTexture(ptrTex);
                mat->updateTextures();
            }
            
            return _material->albedoTexture();
        });
        if (Input::vec2("Scale", albedoScale))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setAlbedoScale(albedoScale);
            }
        }
        if (Input::comboBox("UV Set", uvOptions, albedoUVSet))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setAlbedoUVSet(albedoUVSet);
            }
        }
        
        BasicWidgets::separator("Normal");
        auto normalScale = _material->materialAttributes().normalScale();
        auto normalUVSet = _material->materialAttributes().normalUVSet();
        _normalWidget.selectTexture("##normal", [&](base::Texture* tex) {
            auto ptrTex = std::shared_ptr<base::Texture>(tex);
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setNormalTexture(ptrTex);
                mat->updateTextures();
            }
            return _material->normalTexture();
        });
        if (Input::vec2("Scale##normal", normalScale))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setNormalScale(normalScale);
            }
        }
        if (Input::comboBox("UV Set##normal", uvOptions, normalUVSet))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setNormalUVSet(normalUVSet);
            }
        }
        
        BasicWidgets::separator("Metallic");
        auto metallic = _material->materialAttributes().metalness();
        auto metallicScale = _material->materialAttributes().metalnessScale();
        auto metallicUVSet = _material->materialAttributes().metalnessChannel();
        if (Input::sliderFloat("Value##metallic", &metallic))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setMetalness(metallic);
            }
        }
        _metallicWidget.selectTexture("##metallic", [&](base::Texture* tex) {
            auto ptrTex = std::shared_ptr<base::Texture>(tex);
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setMetalnessTexture(ptrTex);
                mat->updateTextures();
            }
            return _material->metalnessTexture();
        });
        if (Input::vec2("Scale##metallic", metallicScale))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setMetalnessScale(metallicScale);
            }
        }
        if (Input::comboBox("UV Set##metallic", uvOptions, metallicUVSet))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setMetalnessUVSet(metallicUVSet);
            }
        }

        BasicWidgets::separator("Roughness");
        auto roughness = _material->materialAttributes().roughness();
        auto roughnessScale = _material->materialAttributes().roughnessScale();
        auto roughnessUVSet = _material->materialAttributes().roughnessChannel();
        if (Input::sliderFloat("Value##roughness", &roughness))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setRoughness(roughness);
            }
        }
        _roughnessWidget.selectTexture("##roughness", [&](base::Texture* tex) {
            auto ptrTex = std::shared_ptr<base::Texture>(tex);
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setRoughnessTexture(ptrTex);
                mat->updateTextures();
            }
            return _material->roughnessTexture();
        });
        if (Input::vec2("Scale##roughness", roughnessScale))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setRoughnessScale(roughnessScale);
            }
        }
        if (Input::comboBox("UV Set##roughness", uvOptions, roughnessUVSet))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setRoughnessUVSet(roughnessUVSet);
            }
        }
        
        BasicWidgets::separator("Fresnel Tint");
        auto fresnel = _material->materialAttributes().fresnelTint();
        if (Input::colorPicker("Color##fresnel", fresnel))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setFresnelTint(fresnel);
            }
        }
        
        BasicWidgets::separator("Sheen");
        auto sheenIntensity = _material->materialAttributes().sheenIntensity();
        auto sheenColor = _material->materialAttributes().sheenColor();
        if (Input::sliderFloat("Intensity##sheenIntensity", &sheenIntensity, 0.0f, 2.0f))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setSheenIntensity(sheenIntensity);
            }
        }
        if (Input::colorPicker("Color##sheenColor", sheenColor))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setSheenColor(sheenColor);
            }
        }
        
        
        BasicWidgets::separator("Ambient Occlussion");
        auto aoScale = _material->materialAttributes().aoScale();
        auto aoUVSet = _material->materialAttributes().aoUVSet();
        _aoWidget.selectTexture("##ao", [&](base::Texture* tex) {
            auto ptrTex = std::shared_ptr<base::Texture>(tex);
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setAoTexture(ptrTex);
                mat->updateTextures();
            }
            return _material->aoTexture();
        });
        if (Input::vec2("Scale##ao", aoScale))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setAoScale(aoScale);
            }
        }
        if (Input::comboBox("UV Set##ao", uvOptions, aoUVSet))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setAoUVSet(aoUVSet);
            }
        }
    }
    return false;
}

void MaterialEditor::cleanup()
{
    clearWidgets();
    _material.reset();
    _editMaterialList.clear();
}


void MaterialEditor::initWidgets()
{
    if (!_material.get())
    {
        clearWidgets();
    }
    else
    {
        _albedoWidget.setEditTexture(_material->albedoTexture());
        _normalWidget.setEditTexture(_material->normalTexture());
        _metallicWidget.setEditTexture(_material->metalnessTexture());
        _roughnessWidget.setEditTexture(_material->roughnessTexture());
        _aoWidget.setEditTexture(_material->aoTexture());
    }
    
}

void MaterialEditor::clearWidgets()
{
    _albedoWidget.cleanup();
    _normalWidget.cleanup();
    _metallicWidget.cleanup();
    _roughnessWidget.cleanup();
    _aoWidget.cleanup();
}

}
