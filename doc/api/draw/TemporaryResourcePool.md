# TemporaryResourcePool

Header: `<bg2e/draw/pipeline/TemporaryResourcePool.hpp>`, included by draw/all.hpp.
Namespace: `bg2e::draw::pipeline`. This helper is used by temporal accumulation,
denoise, AO/GI/reflections, shadow invocation bindings and fullscreen draws.

The pool belongs to one device and recording thread. `image(command, description)`
leases a compatible native image. Matching includes extent, format, exact usage,
image type, effective mip count and captured native binding generation. Returned
ordinary shared owners, including owners obtained from the image itself, prevent
reuse until released. A raw pointer alone does not retain an image: keep the
returned owner through every GPU reader.

`bindings(command, sharedLayout, set, uniformBytes, identity)` leases one
TemporaryBindings. Matching includes device, retained layout identity, set, uniform
byte size and a four-word invocation identity. Temporal/denoise use slot plus
signal/view/layer; effects/fullscreen/shadows use slot plus invocation ordinal.
Each active invocation has a separate lease, uniform buffer and resource set.
There is no aliasing between live invocations, even inside one command buffer.
`writeUniforms` allocates native storage once, then uploads into retired storage.
Setters retain native input/output owners while the lease is active and compare
resource identity, image generation, binding and type before native rebinding.
`update()` updates changed tuples only. Layouts are retained; input identity is
tracked with weak owners while idle, preventing pointer-address reuse from
certifying stale bindings. Failed setters/updates invalidate tuple equality.

Call `collect()` after releasing a retired acquired slot's invocation leases.
A lease is reusable only after its recording ticket retires and no caller holds
that lease, its descriptor set or its uniform buffer. Image reuse additionally
requires a successfully completed writer and exclusive pool ownership, preserving active histories, previous histories,
caller-held results and later-reader owners. All integrated passes retain sampled
inputs through their invocation lease, including inputs produced by another pool.
Owners are released only when a lease is unreferenced and its recording retires.
No fixed two-image ping-pong or ordered-consumption image aliasing is used.

Recording tickets distinguish completion from abandonment. Submitted work retires
only after its normal CompletionRecord succeeds; unsubmitted work retires only
when its command wrapper is destroyed or successfully reset for a new recording.
A live ended-but-unsubmitted command remains pinned. Abandoned image allocations
are discarded from cache ownership rather than recycled: Vulkan layout tracking
may reflect transitions that never executed. Uniform/set storage may be recycled
after abandonment, since those native descriptor/host writes do not require GPU
execution to become valid. `completed()` still stays
false for abandoned work, preserving acceleration-cache readiness rules. Errors
from submitted GPU work propagate. No wait, polling loop or new drain is added.
A native begin/ticket is required before acquiring temporary resources.

TemporaryPoolLimits defaults to 512 MiB of estimated image/uniform payload,
256 image entries, 256 binding entries and eight idle retirement visits. Completed
excess entries are released; incompatible entries age out. Pending GPU work and
live leases are never overwritten or destroyed to enforce a cache budget, so
outstanding work may temporarily exceed these limits. Completed caller-held
images may leave pool ownership when over budget; their owners remain valid.
Idle age advances on collect calls, not UI-only presentations. Native allocation
alignment and descriptor overhead are not included in the byte estimate.

`statistics()` returns current retained image/binding counts and estimated payload
bytes. Integrating components expose `poolStatistics()` and a synchronized
`setPoolingEnabled(false)` diagnostic. Disabled pools allocate fresh resources
while preserving retirement ownership. TemporalConfiguration and DenoiseConfiguration
expose poolLimits independently of history/invocation capacity. Stable workloads
within the configured cache budget should stop repeating compatible allocations
after all slots and history generations have warmed up. Retaining results indefinitely
or exceeding the budget may require allocations by design.

The pool and device must outlive active recording/submission use. Component cleanup
uses its existing drain, then drops pool ownership without calling cleanup() on
externally held images. Stop producers/discard abandoned recordings before lifecycle
cleanup. Statistics and controls are recording-thread APIs, not concurrent setters.

Profiler counters include temporary.imageAllocations, temporary.imageHits,
temporary.uniformAllocations, temporary.bindingAllocations, temporary.bindingHits,
temporary.bindingUpdates, temporary.unchangedBindings and sampled retained image/
uniform bytes and entry counts. Use poolStatistics for current retention snapshots;
count-style profiler samples aggregate across collection visits and components.
