#!/usr/bin/env python3
"""Phase 2 source contracts and independent arithmetic, never C++/shader/native execution."""
from pathlib import Path
import math
import re
import struct

ROOT = Path(__file__).resolve().parents[3]


def read(name):
    return (ROOT / name).read_text()


def function_body(source, name):
    start = source.index("{", source.index(name))
    end, depth = start + 1, 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def identity():
    return [[float(i == j) for j in range(4)] for i in range(4)]


def multiply(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(4)) for j in range(4)] for i in range(4)]


def transform(a, b):
    return [sum(a[i][k] * b[k] for k in range(4)) for i in range(4)]


def inverse(a):
    augmented = [row[:] + eye for row, eye in zip(a, identity())]
    for column in range(4):
        pivot = max(range(column, 4), key=lambda row: abs(augmented[row][column]))
        augmented[column], augmented[pivot] = augmented[pivot], augmented[column]
        divisor = augmented[column][column]
        assert abs(divisor) > 1e-15
        augmented[column] = [value / divisor for value in augmented[column]]
        for row in range(4):
            if row != column:
                factor = augmented[row][column]
                augmented[row] = [a - factor * b for a, b in zip(augmented[row], augmented[column])]
    return [row[4:] for row in augmented]


def pixel_motion(current, previous, extent):
    assert current[3] > 0 and previous[3] > 0
    return [(previous[i] / previous[3] - current[i] / current[3]) * scale
            for i, scale in [(0, extent[0] * .5), (1, extent[1] * -.5)]]


def half(value):
    return struct.unpack("e", struct.pack("e", value))[0]


def check_sources():
    accumulator = read("lib/src/bg2e/draw/pipeline/TemporalAccumulator.cpp")
    renderer = read("lib/src/bg2e/draw/RendererDeferred.cpp")
    surface = read("lib/src/bg2e/draw/pipeline/TemporalSurfacePass.cpp")
    fsr = read("lib/src/bg2e/draw/FsrUpscaler.cpp")
    uniforms = [("TemporalUniforms", 208), ("TemporalMetadataUniforms", 288),
                ("TemporalReprojectionUniforms", 352)]
    for name, size in uniforms:
        assert f"sizeof({name}) == {size}" in accumulator
    assert "previousView) == 288" in accumulator
    assert "sizeof(SurfaceUniforms) == 288" in surface
    assert "SurfaceBinding{1,8}" in surface and "SurfaceVertexBinding{0,9}" in surface
    assert "limits.maxColorAttachments < 4" in surface and "colorBytes" in surface
    assert "glm::determinant(previous) < 0" in surface
    assert "_temporalSurfaceEnabled = false" in read("lib/include/bg2e/draw/RendererDeferred.hpp")
    assert "previous->metadataMode != metadataMode" in accumulator
    assert "cameraChanged && !rigid" in accumulator
    assert "input.surface.profile == SurfaceProfile::CameraDepth && input.view.reproject" in accumulator
    assert "_recorded->recording->wasSubmitted()" in read("lib/src/bg2e/draw/TemporalCorrespondence.cpp")
    assert "_recorded->sequence < sequence && sequence-_recorded->sequence == 1" in read("lib/src/bg2e/draw/TemporalCorrespondence.cpp")
    assert "metadata.motion.reset()" in accumulator
    assert "invocation.diagnosticMetadata.motion.reset()" in accumulator
    assert "surfaceView ? _surfaceDiagnosticPipeline.get() : _diagnosticPipeline.get()" in accumulator
    for signature in ["TemporalAccumulator::record(", "TemporalAccumulator::recordDiagnostic("]:
        body = function_body(accumulator, signature)
        for forbidden in ["waitIdle(", "immediateSubmit(", "readPixels(", "->completed("]:
            assert forbidden not in body, (signature, forbidden)
    for forbidden in ["waitIdle(", "immediateSubmit(", "readPixels(", "->completed("]:
        assert forbidden not in function_body(fsr, "FsrUpscaler::record("), forbidden
    assert "_previousRecording->wasSubmitted()" in fsr
    assert "input.camera.projection != _recordedProjection" in fsr
    assert "input.jitteredProjection*glm::inverse(input.camera.projection)" in fsr
    assert fsr.index("retained.inputs.push_back(input.opaqueMotion)") < fsr.index("_opaqueMotion.draw(")
    assert "slot.opaqueTemporalSurface.motion,slot.opaqueTemporalSurface.token" in renderer
    assert "slot.camera.view,slot.camera.projection,slot.jitteredProjection,true" in renderer
    for lang in ["glsl", "metal"]:
        core = read(f"shaders/src/draw/lib/temporal.{lang}")
        rigid = read(f"shaders/src/draw/lib/temporal_reprojection.{lang}")
        # HDR confidence, GI validity, freeze/count and guide writes must remain identical.
        assert core[core.index("            float oldCount"):] == rigid[rigid.index("            float oldCount"):]
        for expression in ["previousPosition.xyz-expectedPosition.xyz", "dot(expectedDirection,previousNormal.xyz)",
                           "length(previousCameraPosition.xyz)", "token != 0u && token == previousToken"]:
            assert expression in rigid, expression
        assert rigid.index("accepted = accepted && all(") < rigid.index("metadataPixel =")
        for diagnostic in [False, True]:
            family = "temporal_accumulation_v3_diagnostic" if diagnostic else "temporal_accumulation_v3"
            for profile in ["scalar", "hdr"]:
                wrapper = read(f"shaders/src/draw/{family}/camera_{profile}.comp.{lang}")
                assert "#define TEMPORAL_WORLD 0" in wrapper and "#define TEMPORAL_METADATA 1" in wrapper
                assert f"#define TEMPORAL_SCALAR {int(profile == 'scalar')}" in wrapper
                assert ("#define TEMPORAL_DIAGNOSTIC 1" in wrapper) == diagnostic
                assert f"../lib/temporal_reprojection.{lang}" in wrapper
        vertex = read(f"shaders/src/draw/temporal_surface/surface_v2.vert.{lang}")
        fragment = read(f"shaders/src/draw/temporal_surface/surface_v2.frag.{lang}")
        assert "previousModel" in vertex and "previousClip" in vertex
        assert "previousClip.xy / previousClip.w" in fragment.replace("in.", "")
        assert "65504.0" in fragment and "normalLength <= 1e-6" in fragment
        coverage = "if (((flags & 2u) != 0u && a.a < m.emissionAlpha.y) || ((flags & 4u) != 0u && a.a <= 0.0))"
        assert coverage in fragment and coverage in read(f"shaders/src/draw/raster/gbuffer.frag.{lang}")
        camera = read(f"shaders/src/draw/upscale/motion.frag.{lang}")
        opaque = read(f"shaders/src/draw/upscale/motion_opaque.frag.{lang}")
        marker = "    vec2 size" if lang == "glsl" else "    float2 size"
        assert camera[camera.index(marker):] == opaque[opaque.index(marker):]
        assert "token != 0u" in opaque and "65504.0" in opaque
        display = read(f"shaders/src/draw/temporal_diagnostic/surface.comp.{lang}")
        assert "value.xy/64.0" in display and "options.z == 4u" in display
        assert "value.w > 0.5" in display and "token != 0u" in display
        if lang == "glsl":
            assert "binding=17" in rigid and "binding=18" in rigid
            assert "offset = 288" in rigid
            for offset in [0, 16, 80, 144, 208, 272]:
                assert f"offset = {offset}" in fragment and f"offset = {offset}" in vertex
        else:
            assert "[[texture(15)]]" in rigid and "[[texture(16)]]" in rigid
            assert "sizeof(TemporalUniforms) == 352" in rigid
            assert "[[buffer(9)]]" in vertex and "[[buffer(8)]]" in fragment
    bridge = read("lib/src/bg2e/gpu/vk/FidelityFXBridge.cpp")
    assert "dispatch.motionVectorScale = {1, 1}" in bridge
    flags = re.search(r"description.flags = (.*);", bridge).group(1)
    for flag in ["JITTER_CANCELLATION", "DISPLAY_RESOLUTION_MOTION", "DEPTH_INVERTED"]:
        assert flag not in flags
    owned_files = ["lib/include/bg2e/draw/pipeline/TemporalAccumulator.hpp",
                   "lib/src/bg2e/draw/pipeline/TemporalAccumulator.cpp",
                   "lib/include/bg2e/draw/Upscaler.hpp", "lib/src/bg2e/draw/Upscaler.cpp",
                   "lib/include/bg2e/draw/FsrUpscaler.hpp", "lib/src/bg2e/draw/FsrUpscaler.cpp"]
    owned_files += [str(path.relative_to(ROOT)) for folder in
                    ["shaders/src/draw/temporal_surface", "shaders/src/draw/temporal_accumulation_v3",
                     "shaders/src/draw/temporal_accumulation_v3_diagnostic", "shaders/src/draw/temporal_diagnostic"]
                    for path in (ROOT / folder).iterdir() if path.is_file()]
    owned_files += ["tests/draw/temporal_surface/rigid_probe.hpp",
                    "tests/draw/temporal_surface/correspondence_probe.hpp",
                    "tests/draw/temporal/queued_reprojection_probe.hpp"]
    for name in owned_files:
        source = read(name)
        assert source.startswith("/*") and "GNU General Public License" in source, name
        assert source.count("{") == source.count("}"), name
        assert source.count("(") == source.count(")"), name
        for include in re.findall(r'#include "([^\"]+)"', source):
            assert (ROOT / name).parent.joinpath(include).exists(), (name, include)
        for include in re.findall(r"#include <(bg2e/[^>]+)>", source):
            assert (ROOT / "lib/include" / include).exists(), (name, include)
    assert "recordingCompletion()->abandon()" not in read("tests/draw/temporal_surface/probe.hpp")


def check_arithmetic():
    projection = [[1.2, 0, 0, 0], [0, 1.7, 0, 0],
                  [0, 0, -1000 / 999.9, -100 / 999.9], [0, 0, -1, 0]]
    for extent in [(101, 73), (5, 3)]:
        for jx, jy in [(.25, -.5), (-.375, .125)]:
            offset = identity()
            offset[0][3], offset[1][3] = 2 * jx / extent[0], -2 * jy / extent[1]
            jittered = multiply(offset, projection)
            recovered = multiply(jittered, inverse(projection))
            assert abs(recovered[0][3] * extent[0] * .5 - jx) < 1e-10
            assert abs(-recovered[1][3] * extent[1] * .5 - jy) < 1e-10
            to_unjittered = multiply(projection, inverse(jittered))
            point = [.1, .2, -3, 1]
            clip = transform(jittered, point)
            unjittered = transform(to_unjittered, clip)
            assert max(abs(x) for x in pixel_motion(unjittered, transform(projection, point), extent)) < 1e-10
    # Rigid translation, expected previous world point and sign at odd extent.
    model = identity()
    model[0][3], model[1][3], model[2][3] = .2, -.1, .05
    point = [.1, .2, .5, 1]
    current = transform(model, point)
    previous = transform(inverse(model), current)
    assert max(abs(a - b) for a, b in zip(previous, point)) < 1e-12
    assert math.dist(pixel_motion(current, previous, (101, 73)), [-10.1, -3.65]) < 1e-12
    # Uniform/nonuniform rotated models recover previous points and normals independently.
    angle = .7
    for scale in [(1, 1, 1), (2, 2, 2), (2, 1, .5)]:
        rotation = identity()
        rotation[0][0], rotation[0][2] = math.cos(angle), math.sin(angle)
        rotation[2][0], rotation[2][2] = -math.sin(angle), math.cos(angle)
        scaling = identity()
        for i, value in enumerate(scale):
            scaling[i][i] = value
        model = multiply(rotation, scaling)
        result = transform(inverse(model), transform(model, point))
        assert math.dist(result, point) < 1e-12
        normal_matrix = list(map(list, zip(*inverse(model))))
        current_normal = transform(normal_matrix, [0, 0, 1, 0])
        recovered = transform(list(map(list, zip(*model))), current_normal)
        assert math.dist(recovered, [0, 0, 1, 0]) < 1e-12
    # Perspective divide must follow interpolation; vertex-velocity interpolation is observably wrong.
    vertices = [[-.4, -.2, -1], [.5, -.2, -4], [0, .7, -2]]
    weights = [.2, .3, .5]
    def velocity(point):
        x, y, z = point
        return [(1.2 * (x + .2) / (-z + .1) - 1.2 * x / -z) * 50.5,
                (1.7 * y / (-z + .1) - 1.7 * y / -z) * -36.5]
    denominator = sum(weight / -point[2] for weight, point in zip(weights, vertices))
    point = [sum(weight * point[i] / -point[2] for weight, point in zip(weights, vertices)) / denominator for i in range(3)]
    wrong = [sum(weight * velocity(point)[i] / -point[2] for weight, point in zip(weights, vertices)) / denominator for i in range(2)]
    assert math.dist(velocity(point), wrong) > 1
    assert int(50.5 / 51 * 101) == 100 and int(36.5 / 37 * 73) == 72
    assert abs(.02 + .01 * abs(.5 - 2) - .035) < 1e-12
    # Analytic queued HDR/visibility expectations remain distinguishable from raw restart,
    # including freeze temporarily rolling beyond maxFrames=4 and FP16 storage rounding.
    for slots in [2, 3]:
        exact = [.25, 0., .25, .25]  # visibility, diffuse, reflection confidence, reflection RGB.
        stored = exact[:]
        for scene in range(1, 2 * slots):
            weight = 1 - 1 / min(4, scene + 1)
            for values in [exact, stored]:
                visibility, diffuse, confidence, reflected = values
                next_confidence = confidence * weight + .75 * (1 - weight)
                values[:] = [visibility * weight + .75 * (1 - weight), diffuse * weight + .75 * (1 - weight),
                             next_confidence, (reflected * confidence * weight + .75 * .75 * (1 - weight)) / next_confidence]
            stored[:] = [half(value) for value in stored]
        assert max(abs(a - b) for a, b in zip(exact, stored)) < .003
        assert abs(exact[0] - .75) > .003 and abs(exact[3] - .75) > .003
    assert .5 + .5 / 64 == .5078125 and .5 - .25 / 64 == .49609375


if __name__ == "__main__":
    check_sources()
    check_arithmetic()
    print("PASS: Phase 2 source contracts and independent arithmetic. C++/shader compilation and native GPU/SDK/visual/performance validation: not_run.")
