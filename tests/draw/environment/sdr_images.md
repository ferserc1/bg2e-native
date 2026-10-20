# SDR environment image verification

Manual GPU checks for `EnvironmentGenerator::build(path)` and
`EnvironmentComponent` scene loading. Run on Metal and Vulkan with deployed
environment shaders. Do not add a build target or modify CMake for this check.

1. Load an equirectangular JPEG or PNG with visibly distinct directions. Verify
   all cubemap faces and the skybox retain the expected orientation and colors.
2. Inspect generated sky, irradiance and roughness-prefiltered resources. The
   initial SDR cubemap conversion must decode sRGB after sampling;
   subsequent irradiance and specular passes consume linear radiance. Constant
   linear radiance should integrate to PI in irradiance, as for HDR sources.
3. Load the same pixel pattern as a floating-point `.hdr` source. Ensure its
   decoded float values remain linear and are not sRGB-decoded a second time.
4. Load a scene with Environment pointing at a JPEG. Confirm the scene's active
   environment is used after document publication and the previous document's
   or procedural environment is not left bound.
5. Try uppercase `.JPG`/`.PNG`, corrupt files and missing files. Successful
   input should use the available pixel formats; failed decode should report
   environmentError and retain the last published map.
6. Clear the source image. Procedural sky colors should become active again.

The resource picker accepts HDR, JPEG, PNG, BMP and TGA, matching the image
decoder's implemented formats. No compiled or runtime validation is claimed by
this checklist.
