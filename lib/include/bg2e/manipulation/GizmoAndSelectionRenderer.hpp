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

#pragma once

#include <bg2e/common.hpp>
#include <bg2e/scene/NodeVisitor.hpp>
#include <bg2e/math/all.hpp>
#include <bg2e/scene/Drawable.hpp>
#include <bg2e/render/Engine.hpp>

#include <stack>

namespace bg2e {
namespace manipulation {

class BG2E_API GizmoAndSelectionRenderer : public bg2e::scene::NodeVisitor {
public:
    void init(
        render::Engine * engine,
        VkSampleCountFlagBits sampleCount
    );

    void draw(
        bg2e::scene::Node * sceneRoot,
        const glm::mat4 & viewMatrix,
        const glm::mat4 & projMatrix,
        VkCommandBuffer cmd
    );
    void visit(bg2e::scene::Node * node) override;
    void didVisit(bg2e::scene::Node * node) override;

    inline void setLineIntensity(float i) { _lineIntensity = i; }
    inline float getLineIntensity() const { return _lineIntensity; }

protected:
    render::Engine * _engine = nullptr;

    VkSampleCountFlagBits _sampleCount = VK_SAMPLE_COUNT_1_BIT;

    glm::mat4 _viewProjectionMatrix { 1.0f };
    glm::mat4 _currentTransform { 1.0f };
    std::stack<glm::mat4> _transformStack;

    float _lineIntensity = 0.8f;

    struct FrameData
    {
        glm::mat4 mvp;
        glm::vec4 color;
    };

    glm::mat4 _viewMatrix { 1.0f };
    glm::mat4 _projMatrix { 1.0f };

    VkCommandBuffer _cmdBuffer = VK_NULL_HANDLE;
    VkPipeline _currentBoundPipeline = VK_NULL_HANDLE;

    VkPipeline _selectionPipeline = VK_NULL_HANDLE;
    VkPipelineLayout _selectionPipelineLayout = VK_NULL_HANDLE;

    VkPipeline _gizmoPipeline = VK_NULL_HANDLE;
    VkPipelineLayout _gizmoPipelineLayout = VK_NULL_HANDLE;

    void createSelectionPipeline();
    void createGizmoPipeline();
};

}
}
