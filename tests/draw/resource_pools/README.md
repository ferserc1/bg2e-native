# Retired resource pool probe

Status: source delivered; not compiled or executed. No CMake target is added.
Builds and native execution require explicit user authorization.

Include probe.hpp in a stopped initialized native draw test host and call
bg2e::draw::tests::runTemporaryResourcePoolProbe(engine). False reports missing
sampled/storage formats, not a passing backend. The fixture allocates real images,
uniform buffers and descriptor sets, submits transitions and uses immediateSubmit
for completion. It does not dispatch a shader or compare rendered pixels.

Checks cover distinct live image/descriptor/uniform leases, warm storage identity,
caller-held outputs, later-reader pinning, abandoned versus live recordings,
image extent/layout incompatibility, entry-budget contraction, idle aging and the
fresh allocation diagnostic. The temporal probe also covers abandonment followed
by a retry of the same scene sample, with an old result still retained.

Run the existing temporal and denoise pixel probes on Vulkan and Metal, then
compare normal operation against setPoolingEnabled(false) on each affected pass.
Cover multiple in-flight slots, long GPU latency, many transparent layers/effect
keys, output owners held after beginFrame, freeze-at-limit, history reset, resize,
changing effect counts, new layouts, source destruction and component cleanup.
Never release an output owner before every borrowed GPU reader finishes.

Use TemporalConfiguration/DenoiseConfiguration poolLimits to exercise small byte,
entry and age bounds. Outstanding work may exceed a cache budget; it must remain
pinned. After completion and contraction, excess/idle entries must leave cache
ownership without cleanup() of externally held images. PoolStatistics reports
current retained counts and estimated image/uniform bytes, excluding native
alignment/descriptor overhead. Record allocator/hit/update counters from the
TemporaryResourcePool API documentation with workload size, resolution, layer count,
frames in flight and backend. Stable workloads within cache bounds should stop
repeating compatible allocations after every slot/history generation warms up.

Also exercise successful Vulkan wrapper reset and failed submission. Old recording
tickets can retire unsubmitted storage, but completed() must stay false for aborted
TLAS builds. Submitted readers require normal completion even if wrappers are
released. No speedup or native validation result has been measured yet.
