# Authorized native validation — 2026-10-23

Explicit scope: runTemporalProbe and S10/F0, including necessary library/shader/host
compilation. No CMake source edits, SDK benchmarking or broader validation was requested.
Machine/build identifiers and scoped results: [results.json](results.json).

| Check | Metal, Apple M4 Max | Vulkan/MoltenVK, Apple M4 Max |
|---|---|---|
| runTemporalProbe | passed, exit 0 | passed, exit 0 |
| S10/F0 raster/generation/layer/retained-output assertions | passed, exit 0 | passed, command exit 3 because AO portion unavailable |
| S10/F0 actual AO temporal accumulation, both transparent keys | passed in scoped configuration | unsupported AO path: compute queries, material arrays or scalar target support missing |
| FSR SDK, perspective camera, other S10 sizes/qualities/freeze/limits | not_run | not_run |
| v2/v3/v4/v5 native probes, full S01–S18 matrix, benchmarks | not_run | not_run |

Paired draw/bake SPIR-V and Metal shaders and Debug bg2e library compiled successfully.
Existing warnings included integer narrowing in Value.cpp and UvSurfacePass.cpp;
no compile error occurred. Vulkan reported API 1.3.323 and no Khronos validation layer;
these runs do not establish validation-layer cleanliness. Metal driver is OS bundled;
no separate driver identifier was queried. Both backend runs used the same physical GPU.

S10/F0 uses the checked-in [manual host](../../../tests/draw/temporal/native_host.cpp):
128x64 identity/orthographic view, wall and two overlapping colored semitransparent
planes, maxFrames=32, freeze=true, two completed slots, metadata/adaptation off,
default resampling. Warm up 32 scene frames, move near pane 16 frames, stop for 96
frames, record the following frame (145 total). Generation remained 5 and two exact
layer IDs remained stable. AO and temporal were available in Metal; both transparent
nominal counts reached 32. Count alone is not per-pixel freeze/convergence evidence.
Vulkan retained temporal compute support but lacked the AO capability path; it must
not be described as a passing actual AO/glass temporal test.

The host checks changed final pixels, absent F0 metadata, stable generation/layer
IDs, linear output preservation until stopped slot reuse, and immutable retained
accumulator outputs across reuse. F0 has no token/motion/metric captures. Plane colors
and overlap move as expected in the saved images; no quantitative glass transport,
noise/lag or cross-backend pixel-equivalence criterion was evaluated. Scene indices
31, 47 and 144 are saved as PPM; Metal PNG previews show before/motion-end. Full S10
acceptance across its configuration axes remains pending.

Initial setup attempts and correction:

- The pre-existing build cache used Unix Makefiles with an empty SDK. A separate
  Xcode build directory was configured with installed SDL2 and the SDK's macOS root.
- Sandbox Xcode cache access and GPU discovery failed. Authorized escalated commands
  resolved those access restrictions; the GPU was available outside the sandbox.
- Initial host launch lacked the SDK library rpath; adding it resolved dyld startup.
- Initial S10 host incorrectly asserted immutable renderer linear output *after*
  stopped slot reuse. Renderer.md guarantees it only until that reuse. Corrected the
  fixture to check that interval, and separately retained actual accumulator outputs
  across slot reuse. Production code was unchanged; the corrected Metal run passed.

Exact successful build/host commands, from repository root:

```sh
cmake -S . -B build-temporal-validation -G Xcode \
  -DVULKAN_SDK=/Users/fernando/VulkanSDK/1.4.328.1/macOS \
  -DSDL2_INCLUDE_DIR=/opt/homebrew/opt/sdl2/include/SDL2 \
  -DSDL2_LIBRARY=/opt/homebrew/opt/sdl2/lib/libSDL2.dylib \
  -DBG2E_BUILD_EXAMPLES=OFF -DBG2E_BUILD_TESTS=OFF
cmake --build build-temporal-validation --config Debug --target bg2e --parallel 6
clang++ -std=c++20 tests/draw/temporal/native_host.cpp \
  -Ilib/include -I/opt/homebrew/include -Ilib/third_party/bg2-scene \
  -Ilib/third_party/bg2-io -I/Users/fernando/VulkanSDK/1.4.328.1/macOS/include \
  -Lbin/macos/Debug -lbg2e \
  -Wl,-rpath,/Users/fernando/desarrollo/bg2e-native/bin/macos/Debug \
  -Wl,-rpath,/Users/fernando/VulkanSDK/1.4.328.1/macOS/lib \
  -o /tmp/bg2e-temporal-native-host
/tmp/bg2e-temporal-native-host metal probe bin/macos/shaders /tmp/bg2e-temporal-validation/metal-probe
/tmp/bg2e-temporal-native-host metal s10 bin/macos/shaders /tmp/bg2e-temporal-validation/metal-s10
VK_ICD_FILENAMES=/Users/fernando/VulkanSDK/1.4.328.1/macOS/share/vulkan/icd.d/MoltenVK_icd.json \
  /tmp/bg2e-temporal-native-host vulkan probe bin/macos/shaders /tmp/bg2e-temporal-validation/vulkan-probe
VK_ICD_FILENAMES=/Users/fernando/VulkanSDK/1.4.328.1/macOS/share/vulkan/icd.d/MoltenVK_icd.json \
  /tmp/bg2e-temporal-native-host vulkan s10 bin/macos/shaders /tmp/bg2e-temporal-validation/vulkan-s10
```

Native execution required GPU access outside the sandbox. Logs are saved beside
this file; full local build log is build-temporal-validation/logs/build.log (generated,
not versioned). The source checkout was dirty; base revision and exact host hash are
recorded in results.json. These are correctness checks, not performance measurements.

Next unexecuted native baseline in the plan is S01/F0 static convergence, followed by
the remaining probe/matrix entries. Existing permission is for these two named checks;
it does not authorize the whole matrix or benchmarks. State remains implemented_unverified.
