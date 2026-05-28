# Step 2: Shader Compilation Update

## Objective

Update the CMake shader compilation function to support ray tracing shader stages (`.rgen.glsl`, `.rmiss.glsl`, `.rchit.glsl`) by targeting Vulkan 1.2 / SPIR-V 1.4.

## Why This Is Needed

The current `compile_shaders` function in `cmake/utils.cmake` uses:

```cmake
"${GLSLANG_PATH}" -V "${SH}" -o "${SPV}"
```

The `-V` flag targets Vulkan 1.0 / SPIR-V 1.0 by default. Ray tracing pipeline shaders require at least SPIR-V 1.4 (Vulkan 1.2) because they use the `RayTracingKHR` capability and `GL_EXT_ray_tracing` extension.

## Files to Modify

### `cmake/utils.cmake`

Modify the `compile_shaders` function. The key change is adding `--target-env vulkan1.2` to the glslang invocation:

```cmake
function(compile_shaders
    TARGET_NAME
    VULKAN_SDK_PATH
    SRC_PATH
    DST_PATH
)
    set(COMPILED_SHADERS "")
    set(GLSLANG_PATH "${VULKAN_SDK_PATH}/bin/glslang")
    file(GLOB SHADERS_SRC "${SRC_PATH}/*.glsl")

    file(MAKE_DIRECTORY "${DST_PATH}")

    foreach(SH ${SHADERS_SRC})
        get_filename_component(NAME "${SH}" NAME)
        string(REPLACE ".glsl" "" BASE "${NAME}")
        set(SPV "${DST_PATH}/${BASE}.spv")

        # Determine if this is a ray tracing shader
        string(FIND "${NAME}" ".rgen." IS_RGEN)
        string(FIND "${NAME}" ".rmiss." IS_RMISS)
        string(FIND "${NAME}" ".rchit." IS_RCHIT)
        string(FIND "${NAME}" ".rahit." IS_RAHIT)
        string(FIND "${NAME}" ".rint." IS_RINT)

        if(IS_RGEN GREATER -1 OR IS_RMISS GREATER -1 OR IS_RCHIT GREATER -1 OR IS_RAHIT GREATER -1 OR IS_RINT GREATER -1)
            # Ray tracing shaders need Vulkan 1.2 target
            add_custom_command(
                OUTPUT ${SPV}
                COMMAND "${GLSLANG_PATH}" -V --target-env vulkan1.2 "${SH}" -o "${SPV}"
                DEPENDS ${SH}
                COMMENT "Building RT shader ${SH}"
                VERBATIM
            )
        else()
            # Standard shaders (vertex, fragment, compute)
            add_custom_command(
                OUTPUT ${SPV}
                COMMAND "${GLSLANG_PATH}" -V "${SH}" -o "${SPV}"
                DEPENDS ${SH}
                COMMENT "Building shader ${SH}"
                VERBATIM
            )
        endif()

        list(APPEND COMPILED_SHADERS ${SPV})
    endforeach()

    add_custom_target(${TARGET_NAME}_shaders ALL DEPENDS ${COMPILED_SHADERS})
    add_dependencies(${TARGET_NAME} ${TARGET_NAME}_shaders)
endfunction()
```

Key decisions:
- Detection is based on file name patterns (`.rgen.`, `.rmiss.`, `.rchit.`, `.rahit.`, `.rint.`)
- RT shaders get `--target-env vulkan1.2`, other shaders keep the existing behavior
- This preserves backward compatibility for all existing shaders
- The glob pattern `*.glsl` already picks up any new `.glsl` files, so no changes needed for file discovery

## Why Not Target All Shaders to Vulkan 1.2?

Technically, `--target-env vulkan1.2` is backward compatible — shaders compiled for SPIR-V 1.4 work on Vulkan 1.2+ drivers. However, keeping existing shaders at the Vulkan 1.0 target avoids any potential issues with drivers that only support Vulkan 1.0 (though the engine already requires ray query support which implies Vulkan 1.1+). The selective approach is safer.

## Verification

After this step:
- All existing shaders compile identically (no behavioral change)
- Adding `.rgen.glsl`, `.rmiss.glsl`, `.rchit.glsl` files to `shaders/src/` will trigger compilation with `--target-env vulkan1.2`
- The compiled `.spv` files are placed in `bin/<platform>/shaders/` with the correct naming
