# Step 04: Add Application registration and the experimental run overload

## Application
Preserve existing renderDelegate(), setRenderDelegate(shared_ptr<render::RenderLoopDelegate>), inputDelegate and uiDelegate methods and their existing mutable-reference behavior. Add a separate shared_ptr<draw::RenderLoopDelegate>, drawDelegate() getter and setRenderDelegate(shared_ptr<draw::RenderLoopDelegate>) overload.

Each non-null setter clears the other graphics delegate so the most recent registration selects a single contract. A nullptr registration clears both; add an explicit std::nullptr_t overload to preserve previously valid setRenderDelegate(nullptr) expressions. Do not expose execution implementations through Application.

Because existing mutable getters can bypass setters, execution.validate must inspect both slots and reject simultaneous non-null render and draw delegates. Do not rely only on a stored selection flag.

## Validation
Implement the same checks for either run path:
- Application pointer must exist.
- Exactly the delegate matching the selected path must be present.
- A delegate for the other path is a configuration error, never a fallback.
- Input and UI delegates must be present under the current MainLoop contract, which dereferences them. Do not silently create replacement delegates.
- Experimental Metal selection outside macOS throws a configuration error before SDL or GPU creation.

Use std::invalid_argument for configuration errors. Messages identify both the selected path and the supplied delegate, for example: "MainLoop selected production render, but Application contains a draw::RenderLoopDelegate". Specify a similarly actionable message for a missing delegate or two populated slots.

## New overload and internal execution
Add run(Application*, const draw::EngineConfig&) without a default second argument. It constructs DrawGraphicsExecution with a copy of EngineConfig and uses the same runInternal helper. Add the experimental factory definition and its declaration together.

DrawGraphicsExecution owns draw::Engine and draw::RenderLoop. windowType maps the config backend to gpu::WindowType. It must not initialize Factory merely to perform validation. Provide definitions for every GraphicsExecution operation.

After successful validation, report the intentional milestone boundary before creating the SDL window: add an internal ensureRuntimeAvailable operation invoked before SDL initialization; production is a no-op and draw throws std::logic_error("Experimental draw execution requires milestone 02"). This operation is removed or becomes a no-op once milestone 02 initialization is implemented.

## Compatibility
No new run overload uses ambiguous default parameters. Existing run(Application*) always selects production, even if the app mistakenly registered a draw delegate. Registration mismatch reports an exception rather than selecting a backend implicitly.

## End state
Both public routes are selectable and configuration failures are explicit. The production route runs; the experimental route stops at its documented boundary before allocation. No tests or build invocation.
