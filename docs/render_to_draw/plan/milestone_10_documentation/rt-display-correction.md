# GPU 13 presentation correction

The lead reported a native crash in examples/gpu/13_ray_tracing_pipeline:
`std::invalid_argument: Copy requires distinct, equally sized/formatted color or depth-only subresources in transfer layouts`.
Backend/driver details were not specified. This correction remains associated
with M10-06 validation; no successor milestone is introduced.

The sample generated RGBA8 storage and copied it directly to a presentation
surface normally using BGRA8. GPU copy validation requires identical formats and
extents. A four-byte-per-pixel match and shader red/blue swizzle cannot satisfy
that contract. Drawable extents can also differ from the traced image extent.

The example now samples the tonemapped RGBA image in a fullscreen graphics pass
whose pipeline uses the actual surface color/depth formats. Four new GLSL/Metal
display shaders use explicit sampled-image/sampler bindings and a vertex-ID
triangle with no mesh. UVs retain top-to-top orientation and the sampler adapts
to the drawable extent. The result image has Storage|Sampled usage; graphics depth
testing/writes and culling are disabled. Both ray-generation shaders now store
normal RGB channel order. No GPU copy validation was relaxed or backend API changed.

The Metal RT compute encoder is explicitly ended before image transitions and
the display rendering scope. Resize recreates both RT and display resource sets
against the new output image after the existing device drain. Cleanup retains
existing owners and the previously corrected serialized shared-UBO policy.

Existing CMake shader discovery includes the new stage files; no CMake changes
were made. The tutorial now teaches display setup, sampled presentation, resize
rebinding and normal RGB output instead of cross-format byte-copy compatibility.

Static verification covers declarations/bindings, image usages/layout ordering,
Metal encoder closure, surface format selection, resize rebind, fullscreen UV
orientation and GPL/source style. Maintained documentation link/include checks
pass: 229 pages, 1,913 local links/anchors, 111 includes, zero errors and the two
previously declared illustrative header exceptions. Source delta is confined to
the example main.cpp and six example shader files (four new).

No configure, C++/shader compilation or native execution was performed. The lead
must rebuild the example and its new shaders and validate launch, expected color
orientation and resize on the selected backend. The reported crash is corrected
at source-contract level; native confirmation remains pending.
