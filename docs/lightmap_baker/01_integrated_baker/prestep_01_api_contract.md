# Handoff for Define the public contract

Historical initial handoff for [step-01_api_contract.md](step-01_api_contract.md). Step 01 is now implemented; its actual findings are recorded in [prestep_02_uv_surface.md](prestep_02_uv_surface.md).

Plan correction after step 01: the integrated context owns separate bake `RayTracingScene` instances from step 03 onward. The existing `IntegratedLightmapBaker::update(VkCommandBuffer, vulkan::FrameResources&)` signature remains valid; no step-01 source change is required.

## Initial instructions

Read the plan-wide decisions in `../README.md` and the complete step description. Keep the public API specific to the production `bg2e::render` ray tracing path. Define distinct integrated and standalone lifecycles without requiring the integrated mode to update scene components.

## Original findings template

- Existing local changes affecting this step: TODO
- Additional constraints or approved corrections: TODO

## Contract and next-step deliverables

Read [API_CONTRACT.md](../API_CONTRACT.md) before implementation. The following details are the reviewable scope of the next step:

- Public headers under `lib/include/bg2e/render/`: `BakerContext.hpp`, `LightmapBaker.hpp`, `IntegratedBakerContext.hpp` and `LightmapSettings.hpp`; implementation under `lib/src/bg2e/render/`. Add them to `render/all.hpp`. Use the exact names and settings fields in [API_CONTRACT.md](../API_CONTRACT.md).
- `IntegratedBakerContext::createBaker(std::shared_ptr<scene::Node>, LightmapSettings)` returns `std::unique_ptr<IntegratedLightmapBaker>`. `IntegratedLightmapBaker::update(VkCommandBuffer, vulkan::FrameResources&)` is the recording entry point; common `LightmapBaker` exposes `completedFrames()`, `resetAccumulation()`, `readPixels()` and `image()`.
- A baker retains a shared context reference; Engine and scene lifetime are external. Context construction checks `engine->rayTracingSupported()`. Baker construction checks root identity, Drawable type, submesh/index validity and settings. `resolution`, `accumulationFrames` and `samplesPerPixel` must be positive; error messages name the invalid field. The usable-UV2 check is added with the UV-space pass in step 02, so this step remains compilable with an explicit pending-validation error for unsupported input.
- Add inert implementations where later GPU steps are needed; throw a clear “pass not implemented” error rather than returning a successful empty map. Do not add a render loop or depend on `bg2e::gpu`.
