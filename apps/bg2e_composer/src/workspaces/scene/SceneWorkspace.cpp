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

#include "SceneWorkspace.hpp"

#include "../common/SettingsWindows.hpp"
#include "../common/WorkspaceSlots.hpp"

#include <bg2e/base/PlatformTools.hpp>

#include <bg2e/ui/Text.hpp>
#include <bg2e/draw/RendererDeferred.hpp>

#include <utility>

namespace bg2e_composer::workspaces::scene {

SceneWorkspace::SceneWorkspace(const std::shared_ptr<AppDelegate>& delegate,
    const std::shared_ptr<common::SettingsWindows>& settings)
    : _delegate(delegate), _settings(settings)
{
    _properties.setTitle("Model Properties");
    _sceneProperties.setTitle("Scene");
    _workspace.leftPanelSize().min = 300;
}

void SceneWorkspace::init(bg2e::draw::Engine* engine, bg2e::ui::UserInterface* session,
    uint32_t width, uint32_t height, bg2e::ui::WorkspaceCollection& workspaces)
{
    _ui = session;
    initDrawableBrowser();
    const auto weakResourcesDelegate = _delegate;
    _sceneResources.configure([weakResourcesDelegate]() -> const bg2e::db::SceneResourceAnalysis*
    {
        auto owner = weakResourcesDelegate.lock();
        return owner ? owner->sceneResourceAnalysis() : nullptr;
    }, [weakResourcesDelegate]
    {
        auto owner = weakResourcesDelegate.lock();
        return owner && owner->sceneResourceOperationAvailable();
    }, [weakResourcesDelegate]
    {
        if (auto owner = weakResourcesDelegate.lock())
        {
            owner->analyzeSceneResources();
        }
    }, [weakResourcesDelegate](bool ignoreWarnings)
    {
        if (auto owner = weakResourcesDelegate.lock())
        {
            owner->cleanupSceneResources(ignoreWarnings);
        }
    }, [weakResourcesDelegate]
    {
        auto owner = weakResourcesDelegate.lock();
        return owner ? owner->status() : std::string{};
    });
    _toolbar.setTitle("Scene tools");
    _toolbar.options.noClose = true;
    const auto weakUi = weak_from_this();
    registerActions(workspaces);

    workspaces.configureToolbar(SceneSlot, _toolbar, {
        .left = {},
        .right = {"panel.model-properties", "panel.scene"}
    }, bg2e::base::PlatformTools::settingsPath() / SceneToolbarPreferences);
    if (auto delegate = _delegate.lock())
    {
        _material.init(engine, delegate);
        const auto weak = weak_from_this();
        delegate->onSettleUiInteractions([weak](bool commit)
        {
            auto ui = weak.lock();
            if (!ui)
            {
                return true;
            }
            if (commit)
            {
                return (!ui->_nameEdit || ui->_nameEdit->commit()) &&
                    (!ui->_metadataEdit || ui->_metadataEdit->commit()) &&
                    (!ui->_textEdit || ui->_textEdit->commit()) &&
                    ui->_componentInspector.commitInteraction() && ui->_material.commitInteraction() && ui->_settings->graphics().commitInteraction();
            }
            if (ui->_nameEdit)
            {
                ui->_nameEdit->cancel();
            }
            if (ui->_metadataEdit)
            {
                ui->_metadataEdit->cancel();
            }
            ui->_componentInspector.cancelInteraction();
            if (ui->_textEdit)
            {
                ui->_textEdit->cancel();
            }
            ui->_material.cancelInteraction();
            ui->_settings->graphics().cancelInteraction();
            return (!ui->_nameEdit || !ui->_nameEdit->active()) &&
                (!ui->_metadataEdit || !ui->_metadataEdit->active()) &&
                (!ui->_textEdit || !ui->_textEdit->active()) &&
                !ui->_componentInspector.interactionActive() && !ui->_material.interactionActive();
        });
        delegate->onToolsCleanup([weak]
        {
            if (auto ui = weak.lock())
            {
                ui->_lightmap.cleanup();
                ui->_material.cleanup();
                if (ui->_nameEdit)
                {
                    ui->_nameEdit->cancel();
                    ui->_nameEdit.reset();
                }
                if (ui->_metadataEdit)
                {
                    ui->_metadataEdit->cancel();
                    ui->_metadataEdit.reset();
                }
                ui->_componentInspector.cancelInteraction();
                if (ui->_textEdit)
                {
                    ui->_textEdit->cancel();
                }
                ui->_componentInspector.onEditRequested({});
                ui->_componentInspector.onActionRequested({});
                ui->_settings->graphics().clear();
                ui->_debugOptions = {};
                ui->_debugRendererGeneration = UINT64_MAX;
                ui->_performance.close();
                ui->_sceneTree.setRootNode(nullptr);
                ui->_sceneTree.setSelectionManager(nullptr);
                ui->_sceneTree.onSelect({});
                ui->_sceneTree.onReparent({});
                ui->_componentInspector.setNode(nullptr);
                ui->_componentInspector.setValidationRoot({});
                ui->_componentInspector.onAddRequested({});
                ui->_componentInspector.onRemoveRequested({});
                ui->_componentInspector.onIssueFixRequested({});
                ui->_componentInspector.onDrawComponent({});
                ui->_ui = nullptr;
            }
        });
        delegate->onSelectionChanged([weak]
        {
            if (auto ui = weak.lock())
            {
                if (auto delegate = ui->_delegate.lock())
                {
                    if (ui->_metadataEdit)
                    {
                        ui->_metadataEdit->cancel();
                    }
                    ui->_material.setSelectionManager(delegate->selection());
                }
            }
        });
    }
    _debugWindow.setTitle("Debug");
    _debugWindow.setPosition(340,100);
    _debugWindow.setSize(500,530);
    _debugWindow.close();
    _debugWindow.setDrawFunction([weakUi]
    {
        if (auto owner = weakUi.lock())
        {
            if (auto delegate = owner->_delegate.lock())
            {
                owner->drawDebug(delegate);
            }
        }
    });
    if (auto delegate = _delegate.lock())
    {
        _lightmap.init(delegate, engine, session);
    }
    _importSettingsWindow.setTitle("Import Settings");
    _importSettingsWindow.setSize(420, 260);
    _importSettingsWindow.close();
    _importSettingsWindow.setDrawFunction([weakUi]
    {
        if (auto ui = weakUi.lock())
        {
            if (auto owner = ui->_delegate.lock())
            {
                ui->drawImportSettings(owner);
            }
        }
    });
    _importQueueWindow.setTitle("Import queue");
    _importQueueWindow.setPosition(20, 80);
    _importQueueWindow.setSize(380, 230);
    _importQueueWindow.options.noClose = true;
    _importQueueWindow.setDrawFunction([weakUi]
    {
        if (auto ui = weakUi.lock())
        {
            if (auto owner = ui->_delegate.lock())
            {
                ui->drawImportProgress(owner);
            }
        }
    });
    _fileStatus = std::make_shared<bg2e::ui::StatusItem>();
    _saveStatus = std::make_shared<bg2e::ui::StatusItem>();
    _importStatusItem = std::make_shared<bg2e::ui::StatusItem>();
    _statusBar.addItem(_importStatusItem, bg2e::ui::StatusBar::AlignLeft);
    _statusBar.addItem(_fileStatus, bg2e::ui::StatusBar::AlignLeft);
    _statusBar.addItem(_saveStatus, bg2e::ui::StatusBar::AlignRight);
    _workspace.setToolBarHeight(68);
    _workspace.setup(width, height, &_toolbar, &_properties, &_sceneProperties, nullptr, &_statusBar);
    _workspaceWidth = width;
    _workspaceHeight = height;
    _properties.setDrawFunction([weakUi]
    {
        if (auto owner = weakUi.lock())
        {
            if (auto delegate = owner->_delegate.lock())
            {
                owner->drawTools(delegate);
            }
        }
    });
    _sceneProperties.setDrawFunction([weakUi]
    {
        if (auto owner = weakUi.lock())
        {
            if (auto delegate = owner->_delegate.lock())
            {
                owner->drawSceneEditor(delegate);
            }
        }
    });
}

void SceneWorkspace::draw(uint32_t width, uint32_t height)
{
    _componentInspectorDrawn = false;
    if (_nameEdit)
    {
        _nameEdit->beginFrame();
    }
    if (_metadataEdit)
    {
        _metadataEdit->beginFrame();
    }
    if (_textEdit)
    {
        _textEdit->beginFrame();
    }
    struct SettleMissingControls {
        SceneWorkspace& ui;
        ~SettleMissingControls()
        {
            if (ui._nameEdit)
            {
                ui._nameEdit->endFrame();
            }
            if (ui._metadataEdit)
            {
                ui._metadataEdit->endFrame();
            }
            if (ui._textEdit)
            {
                ui._textEdit->endFrame();
            }
            if (!ui._componentInspectorDrawn)
            {
                ui._componentInspector.cancelInteraction();
            }
        }
    } settleMissing{*this};
    auto delegate = _delegate.lock();
    if (!delegate || !delegate->scene())
    {
        return;
    }
    if (delegate->sceneLoading())
    {
        delegate->drawSceneLoader();
        return;
    }
    if (delegate->imports().busy())
    {
        if (auto job = delegate->imports().active(); job && !job->cancelled.load())
        {
            job->loader.draw();
        }
        _importQueueWindow.draw();
        return;
    }
    _sceneResources.draw();
    _drawableInstances.draw();
    _importSettingsWindow.draw();
    if (auto deferred = dynamic_cast<bg2e::draw::RendererDeferred*>(delegate->renderer());
        deferred && !deferred->environmentError().empty())
    {
        bg2e::ui::Text::text("Environment error: " + deferred->environmentError());
    }
    if (_workspaceWidth != width || _workspaceHeight != height)
    {
        _workspace.resize(width, height);
        _workspaceWidth = width;
        _workspaceHeight = height;
    }
    delegate->setLogicalExtent(width, height);
    bindDocument(delegate);
    const auto* document = delegate->document();
    if (document && document->root)
    {
        _fileStatus->setText(document->sourcePath.empty() ? document->root->name() : document->sourcePath.filename().string());
    }
    else
    {
        _fileStatus->setText("No model open");
    }
    if (document && document->dirty)
    {
        _saveStatus->setText(delegate->status().empty() ? "Unsaved changes" :
            "Unsaved changes — " + delegate->status());
    }
    else if (!delegate->status().empty())
    {
        _saveStatus->setText(delegate->status());
    }
    else if (!delegate->viewpointStatus().empty())
    {
        _saveStatus->setText(delegate->viewpointStatus());
    }
    else
    {
        _saveStatus->setText("Ready");
    }
    _importStatusItem->setText(delegate->importStatus());
    _workspace.draw();
    _settings->graphics().draw();
    _debugWindow.draw();
    if (!_debugWindow.isOpen() && _debugOptions.enabled)
    {
        _debugOptions.enabled = false;
        const auto options = _debugOptions;
        delegate->enqueueRendererUpdate(delegate->rendererGeneration(),[options](bg2e::draw::Renderer& renderer)
        {
            if (auto* deferred = dynamic_cast<bg2e::draw::RendererDeferred*>(&renderer))
            {
                deferred->setDebugOptions(options);
            }
        });
    }
    _performance.draw();
    _settings->drawInterface();
    _lightmap.draw();
}

bg2e::ui::Workspace& SceneWorkspace::layout()
{
    return _workspace;
}

}
