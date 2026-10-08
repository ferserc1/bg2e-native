# Step 05: Finalize lifecycle and source compatibility

## Work
Perform a source-level audit of the extraction, then fix omissions inside app/draw only. This is integration work, not a new test suite or a build task.

Enumerate every old MainLoop engine/loop access and map it to GraphicsExecution. Include shutdown, executeSafeUpdateScene, asyncLoad pause/resume, resize requests, descriptor startup, UI callback capture and delta assignment. There must be no secondary production engine or loop left in MainLoop.

Review existing Application and MainLoop call sites without modifying them. Preserve virtual init(argc, argv) behavior: run does not newly invoke Application::init because existing launchers perform that initialization. Preserve delegate call order and the old milliseconds convention inside the production implementation.

## Ownership and failure paths
Ensure the selected execution outlives UI callbacks and all its initialized resources. Cleanup before destroying the window. Guard cleanup against repeated invocation, including exception unwinding; keep cleanup noexcept in destructors and preserve the original exception.

For failures after partial engine initialization, document the actual guarantees of the unchanged production Engine. Do not invent a claim that all render partial failures are recoverable. Clean only established stages; any pre-existing render failure limitation remains outside the extraction.

Existing detached asyncLoad workers are a known lifecycle limitation. Do not redesign them here. Their scheduled completion must address the same active execution during a run; backend replacement while a run or its workers are active is unsupported.

Do not initialize both execution paths, switch backend at runtime, or replace Factory while live experimental GPU objects exist.

## Documentation
Update the milestone completion notes with final internal names if they differ. Record any production-only protected-member bridge required by existing code. Keep unsupported experimental execution and deferred topics explicit.

## Completion criteria
Production applications need no source changes; MainLoop uses one common event/scheduling loop; backend-dependent work resides in execution implementations; draw declarations and definitions are complete; mismatched configuration throws before allocation; no deprecation, render modifications, macro relocation or tests were introduced.

Runtime compatibility is the project lead's milestone acceptance activity. Do not insert compilation-check instructions or run builds as part of this step.
