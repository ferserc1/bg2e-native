# Scene Component Serialization/Deserialization Plan

## Component Status

| # | Class | typeName | serialize | deserialize | Status |
|---|-------|----------|-----------|-------------|--------|
| 1 | `CameraComponent` | `"Camera"` | Implemented | Empty | **To implement** |
| 2 | `TransformComponent` | `"Transform"` | Implemented | Empty | **To implement** |
| 3 | `LightComponent` | `"LightSource"` | Implemented | Empty | **To implement** |
| 4 | `EnvironmentComponent` | `"Environment"` | Implemented | Empty | **To implement** |
| 5 | `DrawableComponent` | `"Drawable"` | Implemented | Empty | **Pending** (needs Engine* access) |
| 6 | `OrbitCameraComponent` | `"OrbitCameraController"` | Implemented | Implemented | Complete |
| 7 | `PolarTransformControllerComponent` | `"PolarTransformController"` | Implemented | Implemented | Complete |
| 8 | `FixedScaleTransformControllerComponent` | `"FixedScaleTransform"` | Implemented (no data) | Empty | **To implement** (new component, free contract) |
| 9 | `SelectableComponent` | `"Selectable"` | Inherits base | Inherits base | N/A (runtime-only) |

### Dependencies

| Class | serialize | deserialize | Status |
|-------|-----------|-------------|--------|
| `base::Camera` | Implemented | Empty | **To implement** |
| `base::Light` | Implemented | Empty | **To implement** |
| `math::Projection` (base) | Implemented | Empty | **To implement** |
| `math::PerspectiveProjection` | Implemented | Empty | **To implement** |
| `math::OpticalProjection` | Implemented | Empty | **To implement** |

---

## Implementation Details

### 1. TransformComponent::deserialize

**File**: `lib/src/bg2e/scene/TransformComponent.cpp:159`

**Serialized format** (from `serialize`):
```json
{
  "type": "Transform",
  "transformMatrix": [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1]
}
```

**Implementation**:
```cpp
void TransformComponent::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path&)
{
    if (!jsonData || !jsonData->isObject())
        return;

    auto& obj = jsonData->objectValue();
    if (obj.count("transformMatrix"))
    {
        auto matNode = obj["transformMatrix"];
        if (matNode && matNode->isMat4())
        {
            _matrix = matNode->glmMat4Value();
        }
    }
}
```

---

### 2. EnvironmentComponent::deserialize

**File**: `lib/src/bg2e/scene/EnvironmentComponent.cpp:49`

**Serialized format** (from `serialize`):
```json
{
  "type": "Environment",
  "equirectangularTexture": "filename.hdr"
}
```

**Implementation**:
```cpp
void EnvironmentComponent::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path& basePath)
{
    if (!jsonData || !jsonData->isObject())
        return;

    auto& obj = jsonData->objectValue();
    if (obj.count("equirectangularTexture"))
    {
        std::string fileName = obj["equirectangularTexture"]->stringValue("");
        if (!fileName.empty())
        {
            setEnvironmentImage(basePath, fileName);
        }
    }
}
```

---

### 3. CameraComponent::deserialize

**File**: `lib/src/bg2e/scene/CameraComponent.cpp:59`

**Serialized format** (from `serialize`):
```json
{
  "type": "Camera",
  "cameraData": {
    "projection": {
      "type": "PerspectiveProjection",
      "near": 0.1,
      "far": 100.0,
      "fov": 60.0
    }
  }
}
```

**Implementation**:
```cpp
void CameraComponent::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path&)
{
    if (!jsonData || !jsonData->isObject())
        return;

    auto& obj = jsonData->objectValue();
    if (obj.count("cameraData"))
    {
        _camera.deserialize(obj["cameraData"]);
    }
}
```

**Dependency — `base::Camera::deserialize`** (`lib/include/bg2e/base/Camera.hpp:42`):
```cpp
void deserialize(std::shared_ptr<json::JsonNode> jsonData)
{
    if (!jsonData || !jsonData->isObject())
        return;

    auto& obj = jsonData->objectValue();
    if (obj.count("projection"))
    {
        auto projData = obj["projection"];
        if (projData && projData->isObject())
        {
            auto& projObj = projData->objectValue();
            std::string type = projObj.count("type") ?
                projObj["type"]->stringValue("") : "";

            if (type == "PerspectiveProjection")
            {
                auto proj = std::make_shared<math::PerspectiveProjection>();
                proj->deserialize(projData);
                _projection = proj;
            }
            else if (type == "OpticalProjection")
            {
                auto proj = std::make_shared<math::OpticalProjection>();
                proj->deserialize(projData);
                _projection = proj;
            }
        }
    }
    else if (obj.count("projectionMatrix"))
    {
        auto matNode = obj["projectionMatrix"];
        if (matNode && matNode->isMat4())
        {
            _projMatrix = matNode->glmMat4Value();
        }
    }
}
```

**Dependency — `Projection::deserialize`** (`lib/include/bg2e/math/projections.hpp:66`):
```cpp
virtual void deserialize(std::shared_ptr<json::JsonNode> jsonData)
{
    if (!jsonData || !jsonData->isObject())
        return;

    auto& obj = jsonData->objectValue();
    if (obj.count("near"))
    {
        _near = obj["near"]->numberValue(_near);
    }
    if (obj.count("far"))
    {
        _far = obj["far"]->numberValue(_far);
    }
}
```

**Dependency — `PerspectiveProjection::deserialize`** (`lib/include/bg2e/math/projections.hpp:106`):
```cpp
void deserialize(std::shared_ptr<json::JsonNode> jsonData) override
{
    if (!jsonData || !jsonData->isObject())
        return;

    Projection::deserialize(jsonData);

    auto& obj = jsonData->objectValue();
    if (obj.count("fov"))
    {
        _fov = obj["fov"]->numberValue(_fov);
    }
}
```

**Dependency — `OpticalProjection::deserialize`** (`lib/include/bg2e/math/projections.hpp:146`):
```cpp
void deserialize(std::shared_ptr<json::JsonNode> jsonData) override
{
    if (!jsonData || !jsonData->isObject())
        return;

    Projection::deserialize(jsonData);

    auto& obj = jsonData->objectValue();
    if (obj.count("focalLength"))
    {
        _focalLength = obj["focalLength"]->numberValue(_focalLength);
    }
    if (obj.count("frameSize"))
    {
        _frameSize = obj["frameSize"]->numberValue(_frameSize);
    }
}
```

---

### 4. LightComponent::deserialize

**File**: `lib/src/bg2e/scene/LightComponent.cpp:52`

**Serialized format** (from `serialize`):
```json
{
  "type": "LightSource",
  "lightData": {
    "color": [1,1,1,1],
    "intensity": 1.0,
    "type": "OMNI",
    "spotAngle": 22.0,
    "spotCutoff": 14.0,
    "castShadows": true,
    "sourceSize": 0.5,
    "shadowSamples": 8
  }
}
```

**Implementation**:
```cpp
void LightComponent::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path&)
{
    if (!jsonData || !jsonData->isObject())
        return;

    auto& obj = jsonData->objectValue();
    if (obj.count("lightData"))
    {
        _light.deserialize(obj["lightData"]);
    }
}
```

**Dependency — `base::Light::deserialize`** (`lib/include/bg2e/base/Light.hpp:81`):
```cpp
void deserialize(std::shared_ptr<json::JsonNode> jsonData)
{
    if (!jsonData || !jsonData->isObject())
        return;

    auto& obj = jsonData->objectValue();

    if (obj.count("color"))
    {
        auto colorNode = obj["color"];
        if (colorNode && colorNode->isVec4())
        {
            _color = colorNode->colorValue();
        }
    }
    if (obj.count("intensity"))
    {
        _intensity = obj["intensity"]->numberValue(_intensity);
    }
    if (obj.count("type"))
    {
        std::string typeStr = obj["type"]->stringValue("");
        if (typeStr == "OMNI") _type = TypeOmni;
        else if (typeStr == "SPOT") _type = TypeSpot;
        else if (typeStr == "DIRECTIONAL") _type = TypeDirectional;
        else _type = TypeDisabled;
    }
    if (obj.count("spotAngle"))
    {
        _spotAngle = obj["spotAngle"]->numberValue(_spotAngle);
    }
    if (obj.count("spotCutoff"))
    {
        _spotCutoff = obj["spotCutoff"]->numberValue(_spotCutoff);
    }
    if (obj.count("castShadows"))
    {
        _castShadows = obj["castShadows"]->boolValue(_castShadows);
    }
    if (obj.count("sourceSize"))
    {
        _sourceSize = obj["sourceSize"]->numberValue(_sourceSize);
    }
    if (obj.count("shadowSamples"))
    {
        _shadowSamples = static_cast<uint32_t>(
            obj["shadowSamples"]->numberValue(static_cast<int>(_shadowSamples))
        );
    }
}
```

---

### 5. FixedScaleTransformControllerComponent — serialize + deserialize

**File**: `lib/src/bg2e/scene/FixedScaleTransformController.cpp`

New component, no pre-existing JSON contract. The `scale` property must be added to `serialize` and implemented in `deserialize`.

**Updated serialize**:
```cpp
std::shared_ptr<json::JsonNode> FixedScaleTransformControllerComponent::serialize(const std::filesystem::path& basePath)
{
    auto compData = Component::serialize(basePath);
    compData->objectValue()["scale"] = JSON(_scale);
    return compData;
}
```

**Serialized format**:
```json
{
  "type": "FixedScaleTransform",
  "scale": 1.0
}
```

**deserialize**:
```cpp
void FixedScaleTransformControllerComponent::deserialize(std::shared_ptr<json::JsonNode> jsonData, const std::filesystem::path&)
{
    if (!jsonData || !jsonData->isObject())
        return;

    auto& obj = jsonData->objectValue();
    if (obj.count("scale"))
    {
        _scale = obj["scale"]->numberValue(_scale);
    }
}
```

---

### 6. DrawableComponent::deserialize — PENDING

**File**: `lib/src/bg2e/scene/DrawableComponent.cpp:68`

Cannot be implemented yet. `db::loadDrawableBg2()` requires a `render::Engine*` pointer that is not available in the `deserialize` signature. Requires an architectural decision on how to provide the engine during deserialization.

---

## Files to Modify

| File | Changes |
|------|---------|
| `lib/include/bg2e/math/projections.hpp` | Implement `Projection::deserialize`, `PerspectiveProjection::deserialize`, `OpticalProjection::deserialize` |
| `lib/include/bg2e/base/Camera.hpp` | Implement `Camera::deserialize` |
| `lib/include/bg2e/base/Light.hpp` | Implement `Light::deserialize` |
| `lib/src/bg2e/scene/CameraComponent.cpp` | Implement `CameraComponent::deserialize` |
| `lib/src/bg2e/scene/TransformComponent.cpp` | Implement `TransformComponent::deserialize` |
| `lib/src/bg2e/scene/LightComponent.cpp` | Implement `LightComponent::deserialize` |
| `lib/src/bg2e/scene/EnvironmentComponent.cpp` | Implement `EnvironmentComponent::deserialize` |
| `lib/src/bg2e/scene/FixedScaleTransformController.cpp` | Implement `deserialize`, update `serialize` to include `scale` |
| `lib/src/bg2e/scene/DrawableComponent.cpp` | **Pending** — needs Engine* access decision |
