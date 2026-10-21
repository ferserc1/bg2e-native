# Production resource reuse and profiling

Draw retains optimization state independently of application entrypoints. Revision
tracking distinguishes transforms, geometry, materials and alpha/import semantics.
A clean frame must not force full scene rebuilding or drain all GPU work.

Persistent native arrays use the exact 16384-entry capacity. AO/GI/reflection
contracts use v3; query shadows use v5. Filename suffixes such as _256 are shader
templates with specialization constant 0, not the current scene capacity. Native
bindings remain explicit; do not rewrite Vulkan/Metal indices as interchangeable.

Acceleration builds/refits and scratch/resource reuse follow exact recording
completion. Resource leases keep images/bindings alive through submissions;
temporary pools default to 512 MiB, 256 images, 256 bindings and eight idle epochs.
Lazy temporal signatures distinguish camera cuts, material/geometry changes and
in-place content edits. Transparent-region fallbacks and narrow typed updates
retain correctness when reuse is unavailable. Rare synchronous edits wait at a
controlled boundary instead of changing in-flight resources.

CPU scopes, GPU timestamps, work counters and Composer Window → Performance
are independent verification tools. Live charts do not require file capture.
Instrumentation being present is not a measured speedup. Compare unchanged-frame
work counters and diagnostic baselines on the same workload/backend/configuration;
keep unsupported timestamps and missing native measurements explicitly unknown.

- [CPU profiling](cpu_profiling.md)
- [Tracing scene resources](../api/draw/TracingSceneResources.md)
- [Transparent regions](../api/draw/TransparentRegions.md)
- [Temporal histories](../api/draw/TemporalAccumulator.md)
- [GPU submission completion](../api/gpu/Submission_tracking_and_waitIdle.md)
- [Composer](../apps/draw_composer.md)
