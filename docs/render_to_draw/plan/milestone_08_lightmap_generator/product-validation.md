# M08-05 — Standalone product parity and validation record

[Owning step](step_05_product-validation.md) · [Progress](../progress.md) ·
[CLI contract](cli-application.md) · [Batch/output](batch-output.md) ·
[Independent inspector](../../../../tests/bake/product/README.md)

## Evidence and acceptance boundary

M08-04 is accepted by the lead's next-step instruction, under the execution
protocol's continuation convention. Compilation/checks are assumed successful;
no backend, machine, package, scenario or numeric result was supplied. That is
acceptance evidence, not an independently observed Vulkan/Metal output artifact.

This delivery supplies the source parity comparison, independent artifact
inspection and tested synthetic inspection fixtures, deployment/regression matrix
and explicit unresolved runtime coverage. The implementing agent did not configure
CMake, compile C++/shaders, launch an engine application/native probe, build/install
a package or measure GPU performance. Runtime rows below remain delegated to the
lead; acceptance follows the existing execution protocol, with no new approval
flow. M08-06 reviews these records before M08-07 documentation; no M09 deletion is
authorized or performed by this validation step.

## Source-backed parity comparison

Sources are the preserved production app/parser, render StandaloneBakeBatch and
DB LightmapOutputWriter, compared with the new app, bake assembly/context/batch
and bake LightmapOutputWriter. Differences below are explicit product contracts,
not requests to restore older behavior over current engine optimizations.

| Workflow | Production reference | Draw/bake product | Disposition |
|---|---|---|---|
| Model/context | BG2 model, camera-less JSON context | BG2/VWGLB/OBJ, same source-only context role | Required workflow retained; extra formats use shared CPU parsers. |
| Prefab/root identity | Complete subtree, node hierarchy/sibling identity | Single serialized node or full multi-root container; runtime/pick IDs excluded | Preserve all roots, transforms, enabled geometry and independent stable identities. |
| UV2 generation | CLI default true; generates atlas and writes model copies | Same useful default; separate explicit copy control for existing UV2 | Generated atlas requires a copy so exported map remains usable. |
| Bad required target | Per-target skip/warning; zero eligible can succeed | Whole batch rejects malformed required geometry; overlap policy still Warn | Deliberate stricter success contract. No skipped-target false success. |
| Accumulation | Per-target reset/completed updates | Frozen settings, one private serial slot/live baker, release before next | Public standalone probe covers reset, stale generation and failure handling; native run pending. |
| Settings/domain | CLI fields and production bounds | Validated public setters; live typed codec plus CLI-last overrides | Current bake bounds take precedence; arbitrary old values are not promised. |
| AO/GI controls | Fixed CLI AO radius, explicit GI bounce/distance | Explicit AO radius/ray bias/GI/exposure/dilation | Native pass implementations remain shared and unchanged. |
| Byte maps | Byte image output, including RGB32F converted/clamped to bytes | RGB8 linear conversion once; PNG/BMP/TGA verified lossless, explicit JPEG lossy | Use independent decoder for non-PNG formats; do not compare JPEG bytes exactly. |
| Float maps | Production CLI exports bytes | Exact RGBA32F native file carrying RGB32F/alpha one | Requires draw native archive loader; no interchange HDR or half-range cap claim. |
| Mode/UV metadata | Existing AO carrier and older model-copy behavior | Explicit bakedAO/bakedGI, linear UV2, unit scale/channel zero | Independent material-section inspection plus required native reload. |
| Optional model copies | Legacy writer clones/copies file resources | Detached independent clone; complete bool-checked native snapshot | Models retain mesh/submesh placement/materials/joints, not a whole prefab scene. |
| Publication/overwrite | Individual files; old files removed before replacement | Stage complete generation; atomic authoritative manifest LAST | Deliberate output-layout change; previous complete generations survive replacement. |
| Source protection | Legacy reservations/known inputs | Canonical/ancestor/hardlink checks, original OBJ/MTLs, custom file resolver, source signatures | Missing/undeclared custom dependencies reject; no guessed custom export contract. |
| Interrupts | SIGINT at sample boundaries; completed targets can already be written | SIGINT/SIGTERM cooperative cancellation, including final sample before publication | No partial new committed batch. Interrupts after commit do not undo a completed generation. |
| Status/help | 0/1/2/130 and standalone help token anywhere | 0/1/2/130/143; help as the standalone action | Invalid args/config 2; unsupported required RT/output/cleanup 1; cancellation 130/143. |
| Execution/deployment | Production Vulkan Engine, original target | Typed no-camera OffscreenApplication, selected Vulkan/Metal, new target | Both remain registered; no hidden production renderer fallback. |

No exact pixel equality between old and new lighting algorithms is asserted.
Compare controlled required behavior, validity, range and declared material
semantics; use independent fixture tolerances for lighting results.

## Independent artifact validation delivered

The Python standard-library inspector reads **committed manifest paths**, checks
one generation/unique targets and metadata, independently decodes PNG (CRC, five
row filters, zlib termination) and BG2TEX01 float maps (native format, exact
subresources, finite nonnegative RGB and alpha one). Optional BG2 v1.2 material
sections are read using the actual big-endian bg2-io token/string format; referenced
native sidecars are checked for locality, baked linear UV2 semantics and actual
baked RGB equality with the separate map. It never initializes the engine or
rewrites an artifact. It reports decoded ranges, sidecar counts and image SHA256.

Independent expected target sets, selected metadata and asymmetric pixel probes
can check orientation/HDR and known chart-edge values against fixture values.
Example values are synthetic writer-test values, not arbitrary scene lighting.
The tool does not parse BG2 geometry or constitute engine reload, prove coverage
or chart-isolated dilation from metadata, compare original source hashes, validate
cancellation/locks or establish native capability/performance. JPEG/BMP/TGA and
files beyond its explicit 64 MiB limits require another independent decoder.
Those limitations are diagnostics rather than silent partial passes.

Agent-run command:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s tests/bake/product -p 'test_*.py' -v
```

**Observed:** 10 tests passed on temporary synthetic CPU files. Coverage includes
PNG filters/CRC/raw byte order, HDR 100000 (above 65504), independent pixel
expectations, multiple targets, material-only native sidecars, bad payloads/alpha/
metadata, duplicate JSON/identities, mixed generations, missing/escaping/symlink
artifacts and current native enum ordinals. Material-prefix fixtures intentionally
contain no reloadable geometry. This is inspector evidence only. No engine output
was inspected or backend success inferred from these tests.

## Lead native/product matrix

For every performed row record: commit plus relevant working-tree diff, OS/device/
driver/backend, build/configuration and shader root, exact command and exit code,
fixture source hashes, manifest/metadata/independent report paths, native reload
result and any observed limitation. Use scratch output/evidence directories outside
all author assets. Preserve pre-run hashes and previous manifest/referenced-file
hashes for failure/cancel tests. Do not hash only the manifest while ignoring files
it references. No run records were supplied for these rows.

| ID | Required controlled check | Independent acceptance evidence | Current evidence |
|---|---|---|---|
| P01 | Camera-less BG2/VWGLB/OBJ/context, single/multi-root prefab, several targets | Exact target identity/count, retained roots/placement and source-only context; no accidental helper outputs | Source/API review; native pending. |
| P02 | AO/GI; one/several samples; reset/restart; mutate settings/source between jobs | Correct requested completed counts, first sample after reset, stale/no-result rejection; no history carried between targets | Standalone/assembly probes provided; native pending. |
| P03 | RGB8 and RGB32F, mode/UV2/color, asymmetric orientation, edge/nearest-chart padding | Independent decode and fixture comparisons; >65504 writer fixture, finite nonnegative native range; known coverage/edge probes | Inspector synthetic suite passed; real product decode/numerics pending. |
| P04 | Generated/existing UV2, copies on/off, GPU-only/HDR materials and joints | Reload complete model in draw after staging removal; geometry/joints/submesh/mode/UV2 and sidecar pixel checks; source hashes unchanged | Batch/assembly probes and material inspector provided; native geometry reload pending. |
| P05 | Duplicate identities, protected OBJ/MTLs/native sidecars/context/config/custom inputs, symlink/hardlink/ancestor aliases | No writes to protected inputs; correct preflight failure; explicit custom resolver contract | Source/preflight inspection and probes; native/filesystem job matrix pending. |
| P06 | Cancellation before work/final sample/later target; save false/throw, lock contention, no-replace/overwrite and final rename failure | 130/143 or 1 as specified; prior manifest AND referenced products byte-identical; no partial new success | Callback/publication ordering inspected; native/process fault matrix pending. |
| P07 | Typed config/CLI-last overrides, strict/skip unavailable settings, bad ranges/schema/type/renderer and unavailable required RT | Effective exported metadata matches final settings; deterministic diagnostics/exits; no silent unsupported-effect fallback | Parser/config probes supplied; actual executable/native capability pending. |
| P08 | Vulkan and Metal on actual supported hosts | Backend/device-specific P01–P07 records; unsupported hardware reported as unavailable, not a pass | Continuation acceptance only; backend/scenario coverage unspecified. |
| P09 | Fresh and shader-only builds; active bundle/install/Distribution/RuntimeShaders; relocation | Real shader tree/stage libraries from executed location, headless run and independent output report | Shared CMake/package source paths inspected; build/package/relocation pending. |
| P10 | Legacy CLI plus existing interactive/offscreen products, Composer joined imports and integrated bake shutdown | Useful old jobs continue; UI/import/cancel/teardown outcomes and outputs recorded | No production/native code changed here; regression executions pending. |
| P11 | Optimized draw diagnostic/enabled baselines and profiling | Correct images AND work counters; edits invalidate, stable scenes hit caches, aborted work retains owners; unsupported timing reports correctly | Existing probes unchanged; native/counter evidence pending. |

P11 uses existing tests/draw/{tracing_tables,shadow_bindings,acceleration_reuse,
resource_pools,temporal_invalidation,transparent_regions,update_synchronization}
and docs/profiling_and_stats. Include cutoff 0/below/equal 0.02 and glTF MASK,
image/BLAS recreation, overflow, pending/aborted recordings, retained outputs,
source replacement, typed drain behavior and teardown. Performance existence in
source is not a measured speedup. Do not introduce timing waits or compare a
camera-frame average with completed UV samples.

## Static deployment and regression conclusions

New target registration uses the existing bundle_app helper. Its paired refresh
depends on the engine paired shader aggregate, copies both draw/bake trees and
covers shader-only builds where POST_BUILD alone would be stale. RuntimeShaders
installation retains both trees. Distribution recursively discovers executables
under apps, including both CLIs. These are source conclusions; they do not prove
that an actual installed bundle contains the right stage libraries.

No production CLI, native render/draw/gpu source, shader, shared CMake helper,
package script, cache/table/pool/update or profiling implementation is modified in
M08-05. Existing M08 implementation and earlier uncommitted work are preserved.
No defect requiring a native implementation rollback was established by this
static comparison. Historical production documentation now distinguishes the new
manifest/HDR product so its older limits are not mistaken for draw/bake limits.

M08-06 must carry this explicit parity/coverage table and P01–P11 evidence status
into its actual retirement manifest/review. General continuation acceptance does
not identify a backend or prove a named scenario. Resolve remaining required
coverage under the agreed lead acceptance policy; never rewrite unknown evidence
as measured success or silently delete its production behavior reference.
