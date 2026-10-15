# Primitive MRT probe

Manual verification fixture for M01-02. This directory is deliberately not registered in CMake: repository instructions require explicit authorization for build-system edits. The library sources are covered by the existing source glob. No agent build, shader compilation or execution has been performed.

Compile `main.cpp` as a separate executable with the same engine include paths, definitions and link dependencies as an existing GPU example. Alternatively include `probe.hpp` in a temporary local GPU harness and call `bg2e::gpu::tests::runMrtProbe(device, vertexModule, fragmentModule)` outside an active frame/command operation. The helper accepts an already initialized device and paired shader modules.

The standalone executable creates an offscreen GPU context, runs MRT/depth plus the [M01-03 subresource probe](../subresources/README.md), and returns zero only after all checks succeed:

```text
mrt_probe vulkan COMPILED_SHADER_DIRECTORY
mrt_probe metal COMPILED_SHADER_DIRECTORY
```

Prepare the shader directory using your platform shader compiler. For Vulkan:

```sh
"$VULKAN_SDK/bin/glslang" -V -S vert tests/gpu/mrt_probe/probe.vert.glsl -o COMPILED_SHADER_DIRECTORY/probe.vert.spv
"$VULKAN_SDK/bin/glslang" -V -S frag tests/gpu/mrt_probe/probe.frag.glsl -o COMPILED_SHADER_DIRECTORY/probe.frag.spv
```

For Metal, compile `probe.metal` to an AIR file and link it to `COMPILED_SHADER_DIRECTORY/probe.metallib`. The library contains `probeVertex` and `probeFragment`. The runner sets these entry points explicitly; it does not use ShaderLib's stage naming conventions. Vulkan uses `main`. The vertex push constant is one float at offset zero, while the fragment float uses Vulkan offset four and Metal buffer zero; Metal passes each stage's data starting at byte zero.

Expected checks:

- Six RGBA8 attachments contain red, green, blue, yellow, magenta and cyan after a fullscreen draw.
- A later far draw with black output is rejected using depth loaded from the preceding pass. Every color pixel remains unchanged.
- A near draw with depth writes disabled does not replace stored depth; a following intermediate-depth draw succeeds. Every color pixel has the expected quarter intensity.
- Excess pipeline attachment counts, color/depth mismatches and pipeline/rendering count mismatches are rejected before native recording.

Run on available Vulkan and Metal devices with validation enabled. A device with insufficient attachment limits or format support should return a clear unsupported-configuration exception rather than pass. The probe does not require ray tracing or FSR.

Additional lead checks: existing draw triangle, render-to-texture and cubemap examples; per-target blending/write masks; integer clear values and formats; depth-only rendering; unsupported usages/formats; query-only/full-RT/no-RT capability reports. These scenarios have not been executed by the agent.

The optional engine-shader-directory argument additionally runs the [paired library probe](../shader_libraries/README.md), including native pipeline creation, distinct fragment results and compute readback.
