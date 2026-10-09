# Step 02: Implement draw Engine initialization and cleanup

## Initialization
Replace the Engine shell with real GPU object creation. DrawGraphicsExecution prepares Factory exactly once after validation and before SDL window creation; query backend windowType and retain that backend for the run. Ensure Factory is not replaced while objects are live. Keep the public run overload unchanged.

Engine::init receives the prepared Backend. Verify its backendType matches EngineConfig. Borrow sharedInstance; set applicationName using the configured name or MainLoop appId, set debug mode, and create the windowed instance. Create WindowSurface with requested formats, create/choose PhysicalDevice and create Device. Device::create establishes the surface render target; do not duplicate that operation. Construct CleanupManager only after Surface exists.

Use initialization stage flags and clear ownership rules. Accessors require initialized state and return abstract references. Engine backendType reports the configured concrete choice without native casts.

## Cleanup
Stop new draw frame production and coordinate background GPU producers, then wait for actual completion through Device::waitIdle from step 01. Its admission gate reopens when the call returns; keep producers stopped through resource destruction. Drain deferred closures and registered resources, release retained command wrappers, clean Surface before Device, clean Instance after all dependent objects. Destroy C++ wrappers in a safe order and detach borrowed pointers. Idempotent cleanup handles a partially constructed Engine without masking the triggering exception.

Scene targets remain RenderLoop-owned and are cleaned before Engine surface/device destruction. UI is still not initialized in this step.

## Execution boundary
Keep ensureRuntimeAvailable throwing until RenderLoop is operational in step 03, even though Engine is now implemented. This avoids routing users into an undefined half-frame path. Preparing/querying backend must not allocate a window or GPU instance before that boundary.

## End state
Engine initialization and cleanup have real implementations with abstract public API. Production execution remains independent. No tests or build invocation.
