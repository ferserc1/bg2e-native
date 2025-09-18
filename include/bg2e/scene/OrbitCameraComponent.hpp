//
//  OrbitCameraComponent.hpp

#pragma once

#include <bg2e/scene/Component.hpp>
#include <bg2e/math/all.hpp>
#include <bg2e/app/InputManager.hpp>

namespace bg2e {
namespace scene {

class BG2E_API OrbitCameraComponent : public Component {
public:
    BG2E_COMPONENT_TYPE_NAME("OrbitCameraController");
    
    void deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path&) override;
    std::shared_ptr<json::JsonNode> serialize(const std::filesystem::path&) override;

    // Accessors
    // Enabled
    inline bool enabled() const { return _enabled; }
    inline void setEnabled(bool value) { _enabled = value; }

    // Rotation (yaw, pitch)
    inline const glm::vec2& rotation() const { return _rotation; }
    inline void setRotation(const glm::vec2& r) { _rotation = r; }

    // Distance
    inline float distance() const { return _distance; }
    inline void setDistance(float d) { _distance = d; }

    // Center
    inline const glm::vec3& center() const { return _center; }
    inline void setCenter(const glm::vec3& c) { _center = c; }

    // Rotation speed
    inline float rotationSpeed() const { return _rotationSpeed; }
    inline void setRotationSpeed(float s) { _rotationSpeed = s; }

    // Wheel speed
    inline float wheelSpeed() const { return _wheelSpeed; }
    inline void setWheelSpeed(float s) { _wheelSpeed = s; }

    // Min focus
    inline float minFocus() const { return _minFocus; }
    inline void setMinFocus(float f) { _minFocus = f; }

    // Pitch limits
    inline float minPitch() const { return _minPitch; }
    inline void setMinPitch(float v) { _minPitch = v; }

    inline float maxPitch() const { return _maxPitch; }
    inline void setMaxPitch(float v) { _maxPitch = v; }

    // Distance limits
    inline float minDistance() const { return _minDistance; }
    inline void setMinDistance(float v) { _minDistance = v; }

    inline float maxDistance() const { return _maxDistance; }
    inline void setMaxDistance(float v) { _maxDistance = v; }

    // Bounds for X, Y, Z
    inline float maxX() const { return _maxX; }
    inline void setMaxX(float v) { _maxX = v; }

    inline float minX() const { return _minX; }
    inline void setMinX(float v) { _minX = v; }

    inline float maxY() const { return _maxY; }
    inline void setMaxY(float v) { _maxY = v; }

    inline float minY() const { return _minY; }
    inline void setMinY(float v) { _minY = v; }

    inline float maxZ() const { return _maxZ; }
    inline void setMaxZ(float v) { _maxZ = v; }

    inline float minZ() const { return _minZ; }
    inline void setMinZ(float v) { _minZ = v; }

    inline float panSpeed() const { return _panSpeed; }
    inline void setPanSpeed(float s) { _panSpeed = s; }
    
    // Displacement speed
    inline float displacementSpeed() const { return _displacementSpeed; }
    inline void setDisplacementSpeed(float v) { _displacementSpeed = v; }
    
    void setRotateButtons(bool left, bool middle, bool right)
    {
        _rotationButtons = { left, middle, right };
    }
    
    void setPanButtons(bool left, bool middle, bool right)
    {
        _panButtons = { left, middle, right };
    }
    
    void setZoomButtons(bool left, bool middle, bool right)
    {
        _zoomButtons = { left, middle, right };
    }
    
    void resizeViewport(const math::Viewport& vp) override;
    void update(float delta) override;
    
    void mouseButtonDown(int button, int x, int y) override;
    void mouseButtonUp(int button, int x, int y) override;
    void mouseMove(int x, int y) override;
    void mouseWheel(int deltaX, int deltaY) override;
    
    void reset();
    
protected:
    struct MouseButtons
    {
        bool left;
        bool middle;
        bool right;
    };
    
    MouseButtons _rotationButtons = { true, false, false };
    MouseButtons _panButtons = { false, false, true };
    MouseButtons _zoomButtons = { false, true, false };
    
    glm::vec2 _rotation = { 0.0f, 0.0f };
    float _distance = 5.0f;
    glm::vec3 _center = { 0.0f, 0.0f, 0.0f };
    float _rotationSpeed = 0.2f;
    float _wheelSpeed = 0.5f;
    float _minFocus = 2.0f;
    
    float _minPitch = -85.0f;
    float _maxPitch = 85.0f;
    float _minDistance = std::numeric_limits<float>::min();
    float _maxDistance = 24.0f;
    
    float _maxX = 45.0f;
    float _minX = -45.0f;
    float _maxY = 45.0f;
    float _minY = -45.0f;
    float _maxZ = 45.0f;
    float _minZ = -45.0f;
    
    float _displacementSpeed = 0.1f;
    
    bool _enabled = true;
    
    float _panSpeed = 1.0f;
    
    bool _mouseButtonPressed = false;
    glm::vec2 _lastPos;
    uint32_t _viewportWidth;
    uint32_t _viewportHeight;
    
    enum OrbitAction {
        None = 0,
        Rotate,
        Pan,
        Zoom
    };
    
    inline bool matchMouseState(
        const app::InputManager::MouseButtonsStatus & status,
        const MouseButtons & buttons
    ) {
        return  status.left == buttons.left &&
                status.middle == buttons.middle &&
                status.rigth == buttons.right;
    }

    OrbitAction getOrbitAction();
};

}
}
