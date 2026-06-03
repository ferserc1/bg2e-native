# Scene Component Deserialization Implementation Plan

## Overview

This document details the implementation plan for completing the `deserialize` methods of scene components that have `serialize` implemented but `deserialize` empty. The plan excludes `DrawableComponent` (handled separately) and `SelectableComponent` (runtime-only).

## Component Status

| # | Class | typeName | serialize | deserialize | Status |
|---|-------|----------|-----------|-------------|--------|
| 1 | `TransformComponent` | `"Transform"` | Implemented | Implemented | **Complete** |
| 2 | `EnvironmentComponent` | `"Environment"` | Implemented | Implemented | **Complete** |
| 3 | `OrbitCameraComponent` | `"OrbitCameraController"` | Implemented | Implemented | **Complete** |
| 4 | `PolarTransformControllerComponent` | `"PolarTransformController"` | Implemented | Implemented | **Complete** |
| 5 | `CameraComponent` | `"Camera"` | Implemented | Empty | **To implement** |
| 6 | `LightComponent` | `"LightSource"` | Implemented | Empty | **To implement** |
| 7 | `FixedScaleTransformControllerComponent` | `"FixedScaleTransform"` | Incomplete (no `scale`) | Empty | **To implement** |
| 8 | `DrawableComponent` | `"Drawable"` | Implemented | Empty | **Excluded** (user handles) |

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

### 1. Light — Serialization String Fix + deserialize

**File**: `lib/include/bg2e/base/Light.hpp`

The `LightType` enum values (`TypeOmni`, `TypeSpot`, `TypeDirectional`) are kept unchanged. Only the serialization strings are updated to match the shader constants (`LIGHT_TYPE_POINT`, `LIGHT_TYPE_SPOT`, `LIGHT_TYPE_DIRECTIONAL`).

#### 1.1 Fix `typeString()` (line 66)

Change the returned strings:
```cpp
std::string typeString() const
{
    switch (_type)
    {
        case TypeOmni:       return "kTypePoint";
        case TypeSpot:       return "kTypeSpot";
        case TypeDirectional: return "kTypeDirectional";
        default:             return "kTypeDisabled";
    }
}
```

#### 1.2 Implement `deserialize()` (line 81)

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
        if (typeStr == "kTypePoint") _type = TypeOmni;
        else if (typeStr == "kTypeSpot") _type = TypeSpot;
        else if (typeStr == "kTypeDirectional") _type = TypeDirectional;
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

**Serialized format**:
```json
{
  "color": [1, 1, 1, 1],
  "intensity": 1.0,
  "type": "kTypePoint",
  "spotAngle": 22.0,
  "spotCutoff": 14.0,
  "castShadows": true,
  "sourceSize": 0.5,
  "shadowSamples": 8
}
```

---

### 2. LightComponent — deserialize

**File**: `lib/src/bg2e/scene/LightComponent.cpp:52`

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

**Serialized format**:
```json
{
  "type": "LightSource",
  "lightData": {
    "color": [1, 1, 1, 1],
    "intensity": 1.0,
    "type": "kTypePoint",
    "spotAngle": 22.0,
    "spotCutoff": 14.0,
    "castShadows": true,
    "sourceSize": 0.5,
    "shadowSamples": 8
  }
}
```

---

### 3. Projection — deserialize (base class)

**File**: `lib/include/bg2e/math/projections.hpp:66`

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

**Serialized format** (base):
```json
{
  "near": 0.1,
  "far": 100.0
}
```

---

### 4. PerspectiveProjection — deserialize

**File**: `lib/include/bg2e/math/projections.hpp:106`

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

**Serialized format**:
```json
{
  "type": "PerspectiveProjection",
  "near": 0.1,
  "far": 100.0,
  "fov": 60.0
}
```

---

### 5. OpticalProjection — deserialize

**File**: `lib/include/bg2e/math/projections.hpp:146`

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

**Serialized format**:
```json
{
  "type": "OpticalProjection",
  "near": 0.1,
  "far": 100.0,
  "focalLength": 50.0,
  "frameSize": 35.0
}
```

---

### 6. Camera — deserialize

**File**: `lib/include/bg2e/base/Camera.hpp:42`

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

**Serialized format** (with projection object):
```json
{
  "projection": {
    "type": "PerspectiveProjection",
    "near": 0.1,
    "far": 100.0,
    "fov": 60.0
  }
}
```

**Serialized format** (with raw matrix):
```json
{
  "projectionMatrix": [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]
}
```

---

### 7. CameraComponent — deserialize

**File**: `lib/src/bg2e/scene/CameraComponent.cpp:59`

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

**Serialized format**:
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

---

### 8. FixedScaleTransformControllerComponent — serialize fix + deserialize

**File**: `lib/src/bg2e/scene/FixedScaleTransformController.cpp`

#### 8.1 Fix `serialize()` (line 32)

Add `scale` to the serialized data:
```cpp
std::shared_ptr<json::JsonNode> FixedScaleTransformControllerComponent::serialize(const std::filesystem::path& basePath)
{
    auto compData = Component::serialize(basePath);
    compData->objectValue()["scale"] = JSON(_scale);
    return compData;
}
```

#### 8.2 Implement `deserialize()` (line 28)

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

**Serialized format**:
```json
{
  "type": "FixedScaleTransform",
  "scale": 1.0
}
```

---

## Implementation Order

| Step | File | Change |
|------|------|--------|
| 1 | `lib/include/bg2e/base/Light.hpp` | Fix `typeString()` strings + implement `deserialize()` |
| 2 | `lib/include/bg2e/math/projections.hpp` | Implement `deserialize()` in `Projection`, `PerspectiveProjection`, `OpticalProjection` |
| 3 | `lib/include/bg2e/base/Camera.hpp` | Implement `Camera::deserialize()` |
| 4 | `lib/src/bg2e/scene/CameraComponent.cpp` | Implement `CameraComponent::deserialize()` |
| 5 | `lib/src/bg2e/scene/LightComponent.cpp` | Implement `LightComponent::deserialize()` |
| 6 | `lib/src/bg2e/scene/FixedScaleTransformController.cpp` | Fix `serialize()` + implement `deserialize()` |

---

## Design Principles

1. **Responsibility chain**: Each class deserializes its own data. `LightComponent` delegates to `Light`, which reads its own fields. `CameraComponent` delegates to `Camera`, which delegates to `Projection`.

2. **Default values**: If a JSON field is missing, the class's default value (from declaration or constructor) is used. No special handling needed — `numberValue(default)` returns the default if the field doesn't exist.

3. **Enum names unchanged**: The `LightType` enum values (`TypeOmni`, `TypeSpot`, `TypeDirectional`) are internal implementation details. Only the serialization strings change to `"kTypePoint"`, `"kTypeSpot"`, `"kTypeDirectional"`.

4. **No numeric type values**: Light type is serialized/deserialized as a string only. Numeric values are internal to the enum and should not appear in JSON files.

## Files NOT Modified

These files reference `LightType` enum values but do **not** need changes because the enum names are unchanged:
- `lib/src/bg2e/ui/LightEditor.cpp`
- `apps/model_edit/src/StageScene.cpp`
- `examples/17_deferred_renderer/src/main.cpp`
- `examples/debug-app/src/SpheresSceneDelegate.cpp`
