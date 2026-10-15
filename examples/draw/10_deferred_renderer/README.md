# Deferred scene acceptance (M01-15)

Target: `draw_deferred_renderer`. macOS binary:
`bin/macos/draw_deferred_renderer.app/Contents/MacOS/draw_deferred_renderer`;
Linux/Windows: corresponding executable under `bin/<platform>`.
The project lead configures/builds the library, new target and paired shaders.
The agent has not configured, compiled, launched or tested this executable.

## Concrete checks

1. Start with the native default, explicit Vulkan and (on macOS) explicit Metal.
   Confirm nested opaque original/blue-tinted clone, two transparent variants,
   floor, HDR sky, camera orbit and four lights. Check material textures, normal
   maps, Fresnel/sheen, floor occlusion and back-to-front glass. There must be no
   missing resources in the bundle/deployed shader tree. Production example 17
   and earlier draw examples remain separate working regression targets.
2. Initialization asserts distinct CPU/GPU mesh/material/texture/image/sampler
   owners and independent albedo values for the original and clone. The scene
   reports that check. Edit clone tint/roughness: original remains unchanged.
   No geometry/texture reload is needed for these scalar live material edits.
3. Turn camera/light animation off, disable Transparent models and choose Material
   debug view in Graphics settings > Final. Inspect the front orange packed cube:
   red metalness alternates 51/255 and 204/255, green roughness is 166/255, blue AO
   is 230/255 (within RGBA8 quantization). Metalness uses UV1 (twice UV0) with
   additional (2,1) scale. Change its channel to Green/Blue/Alpha and inversion;
   expect uniform corresponding values and then 1-value. Restore Red/inversion
   off. Transparent-enabled debug modes show the last processed glass layer,
   matching the current M01 renderer contract rather than a whole-scene debug
   composite. All seven debug modes and Final are real implemented controls.
4. Inspect yellow canopy holes and shadows on the floor. Change cutoff 0 to .5:
   alpha-zero texels at cutoff zero cover/write depth, while .5 discards them and
   query traversal continues through holes. Toggle canopy, shadow enabled,
   light sourceSize/shadowSamples and shadow sample budget. Disable rotating
   lights for a stable comparison. Default transparent objects are receivers,
   excluded from occluders. Run `--no-rt`: no Drawable BLAS/TLAS is required,
   shadows are disabled and ordinary PBR/IBL/glass remains visible. On actual
   no-query hardware, availability/status must report the capability fallback.
   The forced fixture does not alter the hardware capabilities reported by GPU.
5. Default 67% bilinear rendering, 25/50/59/67/100% scales and odd window sizes
   retain valid full output. Pause keeps the image/counters fixed while UI stays
   live; resize/minimize/restore/resume has no dead slot/target references. Final
   exposure/gamma/contrast/brightness occurs once after full HDR composition.
6. Select FSR at startup or in the scene menu on supported Vulkan. Check SDK/
   backend/hardware/active status, sharpness and disable/re-enable behavior.
   Metal/no-SDK/unsupported initialization retains reduced bilinear output with
   reason. Check camera motion/reset/resize/pause; moving lights and glass can
   ghost with the currently camera-only SDK motion. Native Metal upscaling,
   object motion/reactive masks and engine histories are not claimed here.
7. Select Application resampler: its unique live catalog appears without an
   engine enum or registry edit. Toggle enabled: recordedFrames stops increasing,
   built-in fallback continues. Select Bilinear/FSR again: old controls disappear
   and final color/scale/shadow settings survive. CountingGBuffer is a genuine
   injected application pass: opaque increments once per scene, transparent four
   times when both two-submesh glass models are visible, zero when hidden.
8. Replace renderer while paused/running: retained scene/object identity, clone
   edits and final/scale/shadow settings survive. Replacements counter increments
   on request; it is not evidence of successful completion by itself. Queue
   stale edit and replace: old exposure command must be rejected, final exposure
   stays unchanged, and the diagnostic confirms rejection. Try replacement with
   queued file import and resize; no stale pointers/descriptor reuse may occur.
9. Export/reimport settings using the window; compare actual values. Use matching
   `--upscaler` and `--settings=FILE` for startup import. Malformed files fail
   explicitly; unknown/unavailable/type/cross-validation issues are displayed.
   Save preferences then use Restore or `--restore-preferences`. With both CLI
   flags, the explicit document bypasses stored defaults. No automatic settings
   restore is performed without those requests. Persistence retains the existing
   Preferences service's log-only disk durability reporting.
10. Enable validation and cycle all modes/resize/scene edits/cleanup on multiple
    acquired slots. Per-frame rendering must not perform a global idle/readback;
    explicit resource lifecycle/clone/material edit boundaries may drain GPU.
    Exit while edits or replacement are queued, and after initialization failure;
    scene resources must be released while their engine/device remains alive.

## Reference capability checklist

Retained/adapted: OBJ two-submesh geometry with explicit texture assignment,
opaque/ordinary and solid transparency, nested transforms, independent cloning,
floor, camera/light rotation, HDR/sky/IBL, color/debug controls, direct PBR and
optional alpha-aware soft shadows. OBJ MTL loading is not claimed; materials use
provided assets explicitly. Four lights replace the reference's ten because the
new ABI supports at most eight enabled lights.

Deferred to M02: selection/picking/gizmos/material editor integration. Deferred to
M03: advanced AO/GI/reflections, object/reactive motion, engine temporal history,
denoise and optional spatial AA. Deferred to M04/M05: baking/lightmaps/offscreen
accumulation/export. No placeholder controls for these features are present.
The working product is delivered for lead compilation/runtime validation;
M01-16 is the separate code-versus-remaining-plan review after acceptance.

## Cutout-table overflow regression

The canopy and packed-channel cube use one submesh spanning the full cube index
buffer, so their material applies to all six faces. Floor and packed cube have
alphaCutoff=0; only the canopy is an alpha-tested occluder at startup. The
original generic cube has six submeshes: configuring only material(0) previously
left default-cutoff materials/fallback textures on other faces and exceeded the
eight distinct query alpha image/sampler slots.

After rebuilding, start the default example with query shadows enabled and rotate
the camera: it must remain open, checker cutouts must cover every canopy face,
and packed-channel controls must affect every cube face. Toggle canopy/cutoff,
transparent visibility and renderer/upscaler replacement; the fixture needs at
most one alpha image/sampler pair. --no-rt remains useful as a fallback check but
is not the fix. The engine's explicit ninth-distinct-cutout rejection remains
unchanged; no geometry or occluder is dropped. Agent verification is static only.
