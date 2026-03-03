
#pragma once

#include <bg2e/common.hpp>
#include <bg2e/scene/NodeVisitor.hpp>
#include <bg2e/math/all.hpp>
#include <bg2e/scene/Drawable.hpp>
#include <bg2e/render/Engine.hpp>

#include <stack>

namespace bg2e {
namespace manipulation {

class BG2E_API SelectionHighlight : public bg2e::scene::NodeVisitor{
public:
    void init(
        render::Engine * engine
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

    glm::mat4 _viewProjectionMatrix { 1.0f };
    glm::mat4 _currentTransform { 1.0f };
    std::stack<glm::mat4> _transformStack;

    float _lineIntensity = 0.3f;

    struct FrameData
    {
        glm::mat4 mvp;
        glm::vec4 color;
    };

    VkCommandBuffer _cmdBuffer = VK_NULL_HANDLE;
    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;

    void createPipeline();
};

}
}