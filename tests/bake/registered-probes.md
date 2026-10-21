# Registered UV surface and integrated bake probes

The existing test_02_uv_surface and test_03_integrated_baker_context targets now
call the public draw/bake probes through retirement_harness.hpp. Target names and registrations are retained; M09-05 removes the obsolete private
render include directory from test_02. The shared harness uses canonical windowless
OffscreenApplication with no camera frames; local owners release before Engine
cleanup. The agent has not compiled or executed these fixtures.

Run either executable with `[vulkan|metal] [shader-root]`; defaults use the platform
backend and PlatformTools::shaderPath(). The shader root is the deployed directory
containing draw/ and bake/. Invalid arguments, unsupported mandatory capabilities,
missing shaders and failed assertions return failure rather than a successful skip.

- test_02 runs CPU UV geometry/invalid input checks and the GPU UV surface probe:
  coverage/orientation, odd extents, integer identities, world position/normal,
  retained outputs and independent UV2 reload/clone ownership.
- test_03 additionally runs CPU pixel conversion, GPU AO chart/dilation/output and
  GI composition probes, then integrated RTAO and RTGI lifecycle probes on a loaded
  UV target and generated environment. These cover exact internal sample completion,
  rejected invalid frame/generation sequences, reset/cancellation/source validity,
  RGB32F/byte results and retained resource ownership as specified by each probe.

The probes are shared existing tests, not newly asserted native results. Old
private Vulkan harness APIs are removed. M09-05 adds the fixture-specific numerical/material coverage below. M09-06
completes the actual native, no-RT rejection, deployment and regression matrix; target registration alone is
not proof that those checks passed. See uv_surface/, integrated/ and output/ for
the exact assertion bodies and their validation limits.


## M09-05 registered regression port

Both executables now run retirement_uv.hpp: the original two-submesh UV geometry,
per-island nonempty coverage, typed uncovered identity and known covered/empty
texels. The UV1/UV2 diagnostic preview checks its public CPU pixels and uploaded
image, including unit-square boundary edges. Canonical UV v=0 is the first row;
uncovered identity is UINT_MAX rather than the retired pass's zero. The new UV
surface exposes five typed attachments; old four-attachment/depth-manager storage
is intentionally not an API compatibility assertion.

test_03 additionally runs retirement_material.hpp through public GBuffer, queue,
frame/object bindings and GPU Image readPixels. It checks six camera color outputs
and D32 depth, ordinary AO versus explicit linear RGB/UV2 baked flags, the original
230/82/31 RGB fixture in the baked half-float attachment, and scalar material AO
remaining one. The old implicit AO-texture-means-baked rule is replaced by the
canonical explicit IndirectTextureMode contract.

retirement_numeric.hpp preserves the original receiver/overhang geometry and
threshold checks: isolated AO >0.995, selective occlusion differing by >0.08,
neutral uncovered padding, reset sample count, RGB8 packing and finite colored
RGB32F GI with channel separation >0.02. It uses a known colored linear sky and
one live baker/context, globally increasing serials and explicit completion.
Existing integrated/probe.hpp supplies two-sample exact completion, invalid/stale
frame rejection, pending cancellation/reset and retained sample/resource checks.
output/probe.hpp supplies exact means, chart-aware dilation/radius, neutral
padding, HDR/zero-reference normalization, packing and exposure behavior.

The retired rtShadows bake toggle and alpha-as-validity composition assertions
are not recreated: canonical bake settings define indirect-only AO/GI, and result
alpha is one. Direct query-shadow coverage remains in tests/draw/soft_shadows;
validity/chart/dilation is asserted through explicit surface/accumulation metadata.
Legacy fence/descriptor/frame-slot implementation assertions are replaced by the
current completion and ownership contracts. No fixture has been compiled or run
by the agent; thresholds are executable expectations awaiting lead validation.
