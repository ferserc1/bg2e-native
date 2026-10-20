# Renderer initialization recovery

`probe.hpp` provides `runInitializationPreferencesProbe(engine)`. Call with an
already initialized window Engine, stopped producers and no active RenderLoop.
It reproduces an attachment callback restoring an extent-dependent setting after
target allocation, and requires a rebuild before the very first scene update.
Run on Metal and Vulkan using the existing example/probe integration workflow.
No CMake changes are needed. The probe has not been compiled or executed here.

Additional checks with the deployed deferred renderer:

- Save non-default GI, AO and reflection quality, resolution scale and FSR enable
  state. Relaunch with a nonempty scene. No effect may record into old-sized
  targets, and settings must retain their restored values.
- Corrupt the stored settings document or provide out-of-range values. Startup
  must report the restore problem and use the existing defaults, not terminate.
- Remove an optional AO/GI/reflection/temporal/denoise/SMAA shader from a *copy*
  of the deployed shader tree. The relevant component must become unavailable;
  diagnostics must identify it, and the base scene/UI must remain usable.
- Force FSR bridge/context initialization to fail: reconstruction must use the
  bilinear path and expose FSR's status, without terminating the application.
- Inject a final processor or overlay whose initialize throws runtime_error.
  Its partial state must be cleaned, it must not receive later record calls,
  and the initialization warning must identify the omitted feature.
- Remove a default environment generation shader from the copied tree. Startup
  must use validated shader-independent black environment resources and log the
  warning. Test every cubemap face/mip and the BRDF image. A later successful
  environment rebuild must replace that fallback normally.
- Invalid stage contracts, reused lifecycle objects, wrong-device resources,
  invalid formats and unavailable core rendering resources must still fail.
  They must not be misreported as optional-feature recovery.
