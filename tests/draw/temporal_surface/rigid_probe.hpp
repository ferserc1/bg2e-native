/*
 *    business grade graphic engine (bg2e engine)
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
#include <bg2e/draw/pipeline/TemporalSurfacePass.hpp>
#include <bg2e/draw/pipeline/MotionVectorGenerator.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace bg2e::draw::tests {
// Independent current-world-point oracle, not an extension of the camera-only API.
// Validity mirrors the supported affine surface representation, including mirror fallback.
struct RigidSurfaceReference {
    glm::vec3 previousPosition{0}, previousNormal{0};
    glm::vec2 motion{0};
    bool valid = false;
};
inline bool rigidReferenceFinite(const glm::vec4& value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) && std::isfinite(value.w);
}

inline bool rigidReferenceMatrix(const glm::mat4& value, bool affineModel)
{
    for (int c = 0; c < 4; ++c)
    {
        if (!rigidReferenceFinite(value[c]))
        {
            return false;
        }
    }
    const float determinant = glm::determinant(value);
    if (!std::isfinite(determinant) || determinant == 0 || (affineModel && determinant < 0))
    {
        return false;
    }
    const auto inverse = glm::inverse(value);
    for (int c = 0; c < 4; ++c)
    {
        if (!rigidReferenceFinite(inverse[c]))
        {
            return false;
        }
    }
    return !affineModel || (value[0][3] == 0 && value[1][3] == 0 && value[2][3] == 0 && value[3][3] == 1);
}

inline RigidSurfaceReference rigidSurfaceReference(const glm::mat4& currentModel, const glm::mat4& previousModel,
    const pipeline::TemporalSurfaceCamera& camera, const glm::vec3& currentPosition,
    const glm::vec3& currentNormal, gpu::Size2D extent)
{
    if (extent.isZero() || !rigidReferenceMatrix(currentModel,true) || !rigidReferenceMatrix(previousModel,true) ||
        !rigidReferenceMatrix(camera.currentViewProjection,false) || !rigidReferenceMatrix(camera.previousViewProjection,false) ||
        !rigidReferenceFinite(glm::vec4(currentPosition,1)) || !rigidReferenceFinite(glm::vec4(currentNormal,0)))
    {
        return {};
    }
    const auto previous = previousModel*glm::inverse(currentModel)*glm::vec4(currentPosition,1);
    const auto normal = glm::mat3(glm::transpose(glm::inverse(previousModel)))*
        glm::inverse(glm::mat3(glm::transpose(glm::inverse(currentModel))))*currentNormal;
    const auto currentClip = camera.currentViewProjection*glm::vec4(currentPosition,1);
    const auto previousClip = camera.previousViewProjection*previous;
    const auto normalLength = glm::length(normal);
    if (!rigidReferenceFinite(currentClip) || !rigidReferenceFinite(previousClip) || !rigidReferenceFinite(previous) ||
        !rigidReferenceFinite(glm::vec4(normal,normalLength)) || normalLength <= 1e-6f ||
        currentClip.w <= 1e-6f || previousClip.w <= 1e-6f)
    {
        return {};
    }
    const auto motion = (glm::vec2(previousClip)/previousClip.w-glm::vec2(currentClip)/currentClip.w)*
        glm::vec2(0.5f,-0.5f)*glm::vec2(extent.width,extent.height);
    if (!std::isfinite(motion.x) || !std::isfinite(motion.y) || std::abs(motion.x) > 65504 || std::abs(motion.y) > 65504)
    {
        return {};
    }
    return {glm::vec3(previous),normal/normalLength,motion,true};
}

inline float rigidPositionTolerance(float absolute, float relative, const glm::vec3& expectedPrevious,
    const glm::mat4& previousView)
{
    return absolute+relative*glm::length(glm::vec3(previousView*glm::vec4(expectedPrevious,1)));
}

inline void runRigidSurfaceReferenceProbe()
{
    const auto require = [](bool value, const char* message)
    {
        if (!value)
        {
            throw std::runtime_error(message);
        }
    };
    const pipeline::TemporalSurfaceCamera identityCamera;
    const gpu::Size2D extent{101,73};
    const glm::vec3 local(0.1f,0.2f,0.5f), normal(0,0,1);
    const auto current = glm::translate(glm::mat4(1),glm::vec3(0.2f,-0.1f,0.05f));
    const auto translated = rigidSurfaceReference(current,glm::mat4(1),identityCamera,
        glm::vec3(current*glm::vec4(local,1)),normal,extent);
    require(translated.valid && glm::length(translated.previousPosition-local) < 1e-5f &&
        glm::length(translated.motion-glm::vec2(-10.1f,-3.65f)) < 1e-4f,
        "Previous-current framebuffer pixels must preserve sign, odd extent and previous depth");
    const auto previousView = glm::translate(glm::mat4(1),glm::vec3(0,0,-2));
    require(std::abs(rigidPositionTolerance(0.02f,0.01f,glm::vec3(0,0,0.5f),previousView)-0.035f) < 1e-6f,
        "Relative tolerance must use expected previous position in previous camera space");
    // Explicit RH zero-to-one perspective, independent of GLM's default clip-depth setting.
    glm::mat4 projection(0);
    projection[0][0] = 1.2f;
    projection[1][1] = 1.7f;
    projection[2][2] = -1000.0f/999.9f;
    projection[2][3] = -1;
    projection[3][2] = -100.0f/999.9f;
    glm::mat4 jitter(1);
    jitter[3][0] = 0.5f/float(extent.width);
    jitter[3][1] = 1.0f/float(extent.height);
    pipeline::TemporalView now{glm::mat4(1),projection,jitter*projection,true};
    pipeline::TemporalView before{glm::mat4(1),projection,projection,true};
    const glm::vec3 point(0.1f,0.2f,-3);
    const auto stationary = rigidSurfaceReference(glm::mat4(1),glm::mat4(1),{projection,projection},point,normal,extent);
    const auto cameraStationary = pipeline::MotionVectorGenerator::motionPixels(
        pipeline::MotionVectorGenerator::camera(now,before),now.jitteredProjection*glm::vec4(point,1),extent);
    require(stationary.valid && glm::length(stationary.motion) < 1e-5f && glm::length(cameraStationary) < 1e-4f,
        "Stationary camera/object must have zero motion despite current projection jitter");
    now.view[3].x = -0.2f;
    before.view[3].x = 0.05f;
    before.view[3].z = -0.1f;
    const pipeline::TemporalSurfaceCamera camera{projection*now.view,projection*before.view};
    const auto cameraOnly = rigidSurfaceReference(glm::mat4(1),glm::mat4(1),camera,point,normal,extent);
    const auto cameraPixels = pipeline::MotionVectorGenerator::motionPixels(
        pipeline::MotionVectorGenerator::camera(now,before),now.jitteredProjection*now.view*glm::vec4(point,1),extent);
    require(cameraOnly.valid && glm::length(cameraOnly.motion-cameraPixels) < 2e-4f,
        "Rigid reference with a static model must agree with the camera-only production reference");
    const auto previousModel = glm::translate(glm::mat4(1),glm::vec3(-0.1f,0.2f,-0.5f))*
        glm::rotate(glm::mat4(1),-0.3f,glm::vec3(0,1,0));
    const auto localNormal = glm::normalize(glm::vec3(0.2f,0.5f,1));
    for (const auto scale : {glm::vec3(1),glm::vec3(2),glm::vec3(2,1,0.5f)})
    {
        const auto model = glm::rotate(glm::mat4(1),0.7f,glm::vec3(0,1,0))*glm::scale(glm::mat4(1),scale);
        const auto currentPoint = glm::vec3(model*glm::vec4(point,1));
        const auto currentNormal = glm::normalize(glm::mat3(glm::transpose(glm::inverse(model)))*localNormal);
        const auto expected = rigidSurfaceReference(model,previousModel,camera,currentPoint,currentNormal,extent);
        const auto previousPoint = previousModel*glm::vec4(point,1);
        const auto previousNormal = glm::normalize(glm::mat3(glm::transpose(glm::inverse(previousModel)))*localNormal);
        const auto previousClip = camera.previousViewProjection*previousPoint;
        const auto currentClip = camera.currentViewProjection*glm::vec4(currentPoint,1);
        const auto pixelOracle = (glm::vec2(previousClip)/previousClip.w-glm::vec2(currentClip)/currentClip.w)*
            glm::vec2(0.5f,-0.5f)*glm::vec2(extent.width,extent.height);
        require(expected.valid && glm::length(expected.previousPosition-glm::vec3(previousPoint)) < 2e-5f &&
            glm::length(expected.previousNormal-previousNormal) < 2e-5f && glm::length(expected.motion-pixelOracle) < 2e-4f,
            "Rotating plane and uniform/nonuniform scale must recover previous geometry with simultaneous camera motion");
    }
    // Perspective-correct clip interpolation, then divide in the fragment.
    const std::array<glm::vec3,3> triangle{{{-0.4f,-0.2f,-1},{0.5f,-0.2f,-4},{0,0.7f,-2}}};
    const std::array<float,3> weights{0.2f,0.3f,0.5f};
    const auto previousTranslation = glm::translate(glm::mat4(1),glm::vec3(0.2f,0,-0.1f));
    glm::vec3 localInterpolated(0);
    glm::vec4 currentInterpolated(0), previousInterpolated(0);
    glm::vec2 incorrectEndpointMotion(0);
    float reciprocalW = 0;
    for (size_t i = 0; i < triangle.size(); ++i)
    {
        const auto rasterClip = now.jitteredProjection*glm::vec4(triangle[i],1);
        const float weight = weights[i]/rasterClip.w;
        reciprocalW += weight;
        localInterpolated += weight*triangle[i];
        currentInterpolated += weight*(projection*glm::vec4(triangle[i],1));
        previousInterpolated += weight*(projection*previousTranslation*glm::vec4(triangle[i],1));
        const auto vertex = rigidSurfaceReference(glm::mat4(1),previousTranslation,{projection,projection},triangle[i],normal,extent);
        require(vertex.valid,"Perspective fixture vertices must have valid previous clips");
        incorrectEndpointMotion += weight*vertex.motion;
    }
    localInterpolated /= reciprocalW;
    currentInterpolated /= reciprocalW;
    previousInterpolated /= reciprocalW;
    incorrectEndpointMotion /= reciprocalW;
    const auto fragment = rigidSurfaceReference(glm::mat4(1),previousTranslation,{projection,projection},localInterpolated,normal,extent);
    const auto interpolatedMotion = (glm::vec2(previousInterpolated)/previousInterpolated.w-
        glm::vec2(currentInterpolated)/currentInterpolated.w)*glm::vec2(0.5f,-0.5f)*glm::vec2(extent.width,extent.height);
    require(fragment.valid && glm::length(fragment.motion-interpolatedMotion) < 2e-4f &&
        glm::length(fragment.motion-incorrectEndpointMotion) > 1e-3f,
        "Clips must divide after perspective interpolation, not interpolate pre-divided vertex velocities");
    const auto receiverPixel = [](glm::uvec2 pixel, gpu::Size2D signal, gpu::Size2D receiver)
    {
        return glm::uvec2((glm::vec2(pixel)+0.5f)/glm::vec2(signal.width,signal.height)*glm::vec2(receiver.width,receiver.height));
    };
    require(receiverPixel({0,0},{51,37},extent) == glm::uvec2(0,0) &&
        receiverPixel({50,36},{51,37},extent) == glm::uvec2(100,72) &&
        receiverPixel({1,1},{2,2},{3,3}) == glm::uvec2(2,2),
        "Reduced odd signal extents must select receiver pixels from normalized texel centers");
    glm::mat4 singular(1), nonAffine(1), invalid(1);
    singular[0][0] = 0;
    nonAffine[0][3] = 0.25f;
    invalid[1][1] = std::numeric_limits<float>::quiet_NaN();
    const auto mirror = glm::scale(glm::mat4(1),glm::vec3(-1,1,1));
    for (const auto& model : {singular,nonAffine,invalid,mirror})
    {
        require(!rigidSurfaceReference(model,glm::mat4(1),identityCamera,local,normal,extent).valid &&
            !rigidSurfaceReference(glm::mat4(1),model,identityCamera,local,normal,extent).valid,
            "Singular, non-affine, nonfinite and reflected current/previous models must reject");
    }
    require(!rigidSurfaceReference(glm::mat4(1),glm::mat4(1),identityCamera,local,normal,{0,73}).valid &&
        !rigidSurfaceReference(glm::mat4(1),glm::mat4(1),identityCamera,local,glm::vec3(0),extent).valid,
        "Zero extent and degenerate expected normals must reject");
    require(!rigidSurfaceReference(glm::mat4(1),glm::mat4(1),{projection,projection},glm::vec3(0,0,1),normal,extent).valid,
        "A previous/current point behind the perspective camera must not masquerade as valid zero motion");
}
}
