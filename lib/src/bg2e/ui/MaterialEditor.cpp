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
    if (_selectionManager.get())
    {
        return;
    }
    _editMaterialList.clear();
    clearWidgets();
    _material = mat;
    _editMaterialList.push_back(mat);
    initWidgets();
}

std::shared_ptr<render::MaterialBase> MaterialEditor::editMaterial()
{
    return _material;
}

std::shared_ptr<render::MaterialBase> MaterialEditor::editMaterial() const
{
    return _material;
}

void MaterialEditor::clearMaterial()
{
    if (_selectionManager.get())
    {
        return;
    }
    _material.reset();
    _editMaterialList.clear();
    clearWidgets();
}

void MaterialEditor::addEditMaterial(std::shared_ptr<render::MaterialBase>& mat)
{
    if (_selectionManager.get())
    {
        return;
    }

    if (_material.get())
    {
        _editMaterialList.push_back(mat);
    }
    else
    {
        setEditMaterial(mat);
    }
}

void MaterialEditor::setSelectionManager(const std::shared_ptr<manipulation::SelectionManager>& sm)
{
    _selectionManager = sm;
    _selectionManager->onSelect([&]()
    {
        clearWidgets();
        _material.reset();
        _editMaterialList.clear();

        scene::DrawableComponent * drw = nullptr;
        for (const auto& item : _selectionManager->selectedItems())
        {
            auto itemDrawable = item->drawable.lock();
            auto itemMesh = item->mesh.lock();
            if (!drw && itemDrawable)
            {
                drw = itemDrawable.get();
                _material = itemMesh->renderMaterial(item->submesh);
            }

            if (itemDrawable.get() == drw)
            {
                _editMaterialList.push_back(itemMesh->renderMaterial(item->submesh));
            }
        }

        initWidgets();
    });
}

bool MaterialEditor::draw()
{
    if (_material.get() && BasicWidgets::collapsingHeader(_material->materialAttributes().name() + "'s Material Attributes"))
    {
        std::vector<std::string> uvOptions = { "Set 0", "Set 1" };
        std::vector<std::string> channelOptions = { "Red", "Green", "Blue", "Alpha" };
        BasicWidgets::separator("Albedo");
        auto albedoColor = _material->materialAttributes().albedo();
        auto albedoScale = _material->materialAttributes().albedoScale();
        auto albedoUVSet = _material->materialAttributes().albedoUVSet();
        if (Input::colorPicker("Color##albedo", albedoColor))
        {
            for (auto mat : _editMaterialList)
            {
                mat->materialAttributes().setAlbedo(albedoColor);
                notifyOnChange();
            }
        }
        _albedoWidget.selectTexture("##albedo", [&](base::Texture* tex) {
            auto ptrTex = std::shared_ptr<base::Texture>(tex);
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setAlbedoTexture(ptrTex);
                mat->updateTextures();
                notifyOnChange();
            }
            
            return _material->albedoTexture();
        });
        if (Input::vec2("Scale", albedoScale))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setAlbedoScale(albedoScale);
            }
            notifyOnChange();
        }
        if (Input::comboBox("UV Set", uvOptions, albedoUVSet))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setAlbedoUVSet(albedoUVSet);
            }
            notifyOnChange();
        }
        
        auto isTransparent = _material->materialAttributes().isTransparent();
        if (BasicWidgets::checkBox("Is Transparent", &isTransparent))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setIsTransparent(isTransparent);
            }
            notifyOnChange();
        }

        auto refractionFactor = _material->materialAttributes().refractionFactor();
        if (Input::sliderFloat("Refraction##refractionFactor", &refractionFactor, 0.0f, 1.0f))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setRefractionFactor(refractionFactor);
            }
            notifyOnChange();
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
            notifyOnChange();
            return _material->normalTexture();
        });
        if (Input::vec2("Scale##normal", normalScale))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setNormalScale(normalScale);
            }
            notifyOnChange();
        }
        if (Input::comboBox("UV Set##normal", uvOptions, normalUVSet))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setNormalUVSet(normalUVSet);
            }
            notifyOnChange();
        }
        
        BasicWidgets::separator("Metallic");
        auto metallic = _material->materialAttributes().metalness();
        auto metallicScale = _material->materialAttributes().metalnessScale();
        auto metallicChannel = _material->materialAttributes().metalnessChannel();
        auto metallicInvert = _material->materialAttributes().metalnessInvert();
        auto metallicUVSet = _material->materialAttributes().metalnessUVSet();
        if (Input::sliderFloat("Value##metallic", &metallic))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setMetalness(metallic);
            }
            notifyOnChange();
        }
        _metallicWidget.selectTexture("##metallic", [&](base::Texture* tex) {
            auto ptrTex = std::shared_ptr<base::Texture>(tex);
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setMetalnessTexture(ptrTex);
                mat->updateTextures();
            }
            notifyOnChange();
            return _material->metalnessTexture();
        });
        if (Input::comboBox("Channel##metallic", channelOptions, metallicChannel))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setMetalnessChannel(metallicChannel);
            }
            notifyOnChange();
        }
        if (BasicWidgets::checkBox("Invert##metallic", &metallicInvert))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setMetalnessInvert(metallicInvert);
            }
            notifyOnChange();
        }
        if (Input::vec2("Scale##metallic", metallicScale))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setMetalnessScale(metallicScale);
            }
            notifyOnChange();
        }
        if (Input::comboBox("UV Set##metallic", uvOptions, metallicUVSet))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setMetalnessUVSet(metallicUVSet);
            }
            notifyOnChange();
        }

        BasicWidgets::separator("Roughness");
        auto roughness = _material->materialAttributes().roughness();
        auto roughnessScale = _material->materialAttributes().roughnessScale();
        auto roughnessChannel = _material->materialAttributes().roughnessChannel();
        auto roughnessInvert = _material->materialAttributes().roughnessInvert();
        auto roughnessUVSet = _material->materialAttributes().roughnessUVSet();
        if (Input::sliderFloat("Value##roughness", &roughness))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setRoughness(roughness);
            }
            notifyOnChange();
        }
        _roughnessWidget.selectTexture("##roughness", [&](base::Texture* tex) {
            auto ptrTex = std::shared_ptr<base::Texture>(tex);
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setRoughnessTexture(ptrTex);
                mat->updateTextures();
            }
            notifyOnChange();
            return _material->roughnessTexture();
        });
        if (Input::comboBox("Channel##roughness", channelOptions, roughnessChannel))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setRoughnessChannel(roughnessChannel);
            }
            notifyOnChange();
        }
        if (BasicWidgets::checkBox("Invert##roughness", &roughnessInvert))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setRoughnessInvert(roughnessInvert);
            }
            notifyOnChange();
        }
        if (Input::vec2("Scale##roughness", roughnessScale))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setRoughnessScale(roughnessScale);
            }
            notifyOnChange();
        }
        if (Input::comboBox("UV Set##roughness", uvOptions, roughnessUVSet))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setRoughnessUVSet(roughnessUVSet);
            }
            notifyOnChange();
        }
        
        BasicWidgets::separator("Fresnel Tint");
        auto fresnel = _material->materialAttributes().fresnelTint();
        if (Input::colorPicker("Color##fresnel", fresnel))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setFresnelTint(fresnel);
            }
            notifyOnChange();
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
            notifyOnChange();
        }
        if (Input::colorPicker("Color##sheenColor", sheenColor))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setSheenColor(sheenColor);
            }
            notifyOnChange();
        }
        
        
        BasicWidgets::separator("Ambient Occlussion");
        auto aoScale = _material->materialAttributes().aoScale();
        auto aoChannel = _material->materialAttributes().aoChannel();
        auto aoUVSet = _material->materialAttributes().aoUVSet();
        _aoWidget.selectTexture("##ao", [&](base::Texture* tex) {
            auto ptrTex = std::shared_ptr<base::Texture>(tex);
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setAoTexture(ptrTex);
                mat->updateTextures();
            }
            notifyOnChange();
            return _material->aoTexture();
        });
        if (Input::comboBox("Channel##ao", channelOptions, aoChannel))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setAoChannel(aoChannel);
            }
            notifyOnChange();
        }
        if (Input::vec2("Scale##ao", aoScale))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setAoScale(aoScale);
            }
            notifyOnChange();
        }
        if (Input::comboBox("UV Set##ao", uvOptions, aoUVSet))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setAoUVSet(aoUVSet);
            }
            notifyOnChange();
        }

        BasicWidgets::separator("Light Emission");
        auto lightEmission = _material->materialAttributes().lightEmission();
        auto lightEmissionScale = _material->materialAttributes().lightEmissionScale();
        auto lightEmissionChannel = _material->materialAttributes().lightEmissionChannel();
        auto lightEmissionInvert = _material->materialAttributes().lightEmissionInvert();
        auto lightEmissionUVSet = _material->materialAttributes().lightEmissionUVSet();
        if (Input::sliderFloat("Value##lightEmission", &lightEmission, 0.0f, 100.0f))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setLightEmission(lightEmission);
            }
            notifyOnChange();
        }
        _lightEmissionWidget.selectTexture("##lightEmission", [&](base::Texture* tex) {
            auto ptrTex = std::shared_ptr<base::Texture>(tex);
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setLightEmissionTexture(ptrTex);
                mat->updateTextures();
            }
            notifyOnChange();
            return _material->lightEmissionTexture();
        });
        if (Input::comboBox("Channel##lightEmission", channelOptions, lightEmissionChannel))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setLightEmissionChannel(lightEmissionChannel);
            }
            notifyOnChange();
        }
        if (BasicWidgets::checkBox("Invert##lightEmission", &lightEmissionInvert))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setLightEmissionInvert(lightEmissionInvert);
            }
            notifyOnChange();
        }
        if (Input::vec2("Scale##lightEmission", lightEmissionScale))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setLightEmissionScale(lightEmissionScale);
            }
            notifyOnChange();
        }
        if (Input::comboBox("UV Set##lightEmission", uvOptions, lightEmissionUVSet))
        {
            for (auto & mat : _editMaterialList)
            {
                mat->materialAttributes().setLightEmissionUVSet(lightEmissionUVSet);
            }
            notifyOnChange();
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
        _lightEmissionWidget.setEditTexture(_material->lightEmissionTexture());
    }
    
}

void MaterialEditor::clearWidgets()
{
    _albedoWidget.cleanup();
    _normalWidget.cleanup();
    _metallicWidget.cleanup();
    _roughnessWidget.cleanup();
    _aoWidget.cleanup();
    _lightEmissionWidget.cleanup();
}

}
