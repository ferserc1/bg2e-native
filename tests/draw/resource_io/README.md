# Complete draw resource IO probe

`probe.hpp` provides `bg2e::draw::tests::runCompleteResourceIoProbe(engine, directory)`.
Invoke it manually from an initialized draw application at a stopped submission
boundary, using a disposable existing output directory. It is not a CMake target
and has not been compiled or run by the implementing agent.

The probe checks quoted/backslash/newline JSON strings; a source-less GPU float
texture with HDR values and two authored mips; native archive bytes; bakedGI/UV2,
scale, linear color and sampler metadata after complete scene save/reload; and
failure rollback when a custom component has no reference serializer. It also
checks that temporary Drawable paths are restored. Run on available Vulkan and
Metal backends, with appropriate deployment/runtime resources.

Additional project-lead checks:

1. Open BG2/VWGLB with distinct submesh/material names, duplicate material names
   with independent edits, group/visibility and input/output joints. Save/reload
   and compare geometry, normals, both UV sets and metadata.
2. Import OBJ with multiple shape/material runs and a nested MTL directory;
   compare base/PBR/normal/AO maps, clamp/UV scale and missing-file failures.
3. Import indexed static glTF/GLB, including embedded images, transforms, two UV
   sets, packed metallic/roughness and alpha masks. Unsupported animation,
   extensions, UV transforms and missing resources must report errors.
4. Save a scene with registered custom schema/reference/draw hooks; compare its
   custom fields and chain-joint metadata. Raw/schema-only probes remain in
   `tests/draw/db_levels`; unknown typed components must fail explicitly.
5. Inspect all `.bg2tex` mip/layer payloads for an authored cubemap archive and
   load supported 2D material archives on both backends. Verify truncated/header/
   length/trailing-byte rejection and uninitialized/unsupported image rejection.
6. Open/import/drop files in draw_model_edit; cancel dialogs and dirty prompts;
   exercise Save/Discard/Cancel for open and close. Force missing inputs,
   unsupported BG2 graph exports and unwritable destinations. Failed load/save
   must retain the current document and its dirty state. A successfully saved
   import becomes clean and has a native save path.
7. Save over an existing scene and inspect its previous resources after success
   and failure. New sidecars are unique; previous generations remain. Move the
   main file together with its sidecars, reopen, and verify relative paths.
8. Confirm production model_edit and production PNG/HDR saves still work. Draw
   `.bg2tex` sidecars intentionally require the draw loader.
