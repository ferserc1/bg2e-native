# M08-03 batch/output probes

These are opt-in fixtures, not CMake targets. They have not been compiled or run
by the implementing agent. The lead owns compilation and native execution.

Include `probe.hpp` in a local driver, preserving Engine/Device lifetime and
stopping every scene/resource producer. Scratch destinations must be fresh,
caller-owned directories outside all protected assets. Do not use packaging or
production assets as writable fixtures.

- `runBakePublicationProbe(scratch)` exercises CPU archive exact float bits
  (100000 > 65504), invalid archive rejection before file creation, OS no-clobber
  publication and explicit replacement. No graphics initialization is needed.
- `runLightmapOutputProbe(engine,target,inputModel,scratch)` needs an initialized
  stopped offscreen draw Engine and a loaded standard UV2 target in a private
  scene. It exercises complete model export/reload, linear UV2 bakedGI and exact
  float map bytes, RGB8 lossless decode, no publication during staging, duplicate
  writes/identities, incomplete/repeated commit, overwrite rejection, protected
  manifest rejection, changed scene rejection, abandoned replacement and source
  asset/material preservation. The fixture assumes standard builtin components;
  use a resolver for custom-component fixtures.

Additional integration matrix for the lead (both supported native backends):

1. Run StandaloneBakeBatch with two model/prefab targets, AO and GI, one and several
   accumulation frames, byte and float outputs, model copies on/off. Reload only
   paths listed by `lightmaps.json`; assert counts/UV2/mode/orientation/dilation.
2. Return false from progress at target zero/sample zero, after one completed
   sample, at the final completed sample, and on target two after target one has
   staged. No new manifest may appear; an existing manifest and its referenced
   generation must remain byte-identical. `Result` must be cancelled/count zero.
3. Change transform/material/texture/mesh/node membership in a callback; expect
   rejection without publication. Change `Options.current` validity and check
   cancellation. Changes to frozen caller settings must not alter the job.
4. Protect OBJ files with multiple MTL files, BG2/native sidecars, environment
   images, context and prefab JSON, disabled-node sources, custom resolver files
   and explicit protected directories. Exercise manifest symlinks, hardlink
   aliases and output namespace intersections. Undeclared custom subclasses,
   missing inputs and resolver directory-only declarations must reject.
5. Simulate image/model save false/exception, unwritable stage and manifest rename
   failure. Existing outputs must survive. A rename failure after generation
   preparation may leave an unreferenced complete generation, never a partially
   referenced manifest. Check stale file size/mtime and same-size content edits.
6. Two writers targeting the same directory must contend on `.lightmaps.lock`.
   Test no-replace destination races, explicit overwrite, filesystem/platform
   errors and unsupported no-replace operations. No hardlink support is required
   on macOS/Linux/Windows native publication paths.
7. Validate deferred target hooks once, already loaded custom hooks not repeated,
   foreign Engine rejection, CPU/uploaded geometry mismatch and resource failure.
   Required unsupported tracing must fail without a newly published product.

Crash recovery is manual: after ensuring no writer process is alive, inspect and
remove stale lock/staging directories. Prior generation cleanup is outside this
step. No power-loss durability or hostile concurrent filesystem mutation guarantee
is claimed. Callbacks and dependency resolvers are stopped CPU-only callbacks.
