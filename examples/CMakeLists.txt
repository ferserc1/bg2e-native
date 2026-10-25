# bg2e::gpu examples
add_subdirectory(gpu/01_instance)
add_subdirectory(gpu/02_device)
add_subdirectory(gpu/03_offscreen_device)
add_subdirectory(gpu/04_clear_loop)
add_subdirectory(gpu/05_simple_triangle)
add_subdirectory(gpu/06_offscreen_triangle)
add_subdirectory(gpu/07_uniform_buffers)
add_subdirectory(gpu/08_render_to_texture)
add_subdirectory(gpu/09_cubemap)
add_subdirectory(gpu/10_cubemap_render_pass)
add_subdirectory(gpu/11_ray_query_shadows)
add_subdirectory(gpu/12_deferred_cleanup)
add_subdirectory(gpu/13_ray_tracing_pipeline)

# bg2e::draw examples
add_subdirectory(draw/01_window_ui)
add_subdirectory(draw/02_simple_triangle)
add_subdirectory(draw/03_uniform_buffers)
add_subdirectory(draw/04_render_to_texture)
add_subdirectory(draw/05_cubemap)
add_subdirectory(draw/06_cubemap_render_pass)
add_subdirectory(draw/07_ray_query_shadows)
add_subdirectory(draw/08_deferred_cleanup)
add_subdirectory(draw/09_ray_tracing_pipeline)

# M01 deferred scene product over the draw API.
add_subdirectory(draw/10_deferred_renderer)

# M02 interactive selection and material modification over the draw API.
add_subdirectory(draw/11_manipulation)

# M04 integrated baking and applied indirect maps over the draw API.
add_subdirectory(draw/12_lightmaps)

# M05 headless scene sampling, settings import and PNG output.
add_subdirectory(draw/13_offscreen_application)

# Reflection tutorial: built-in and application-owned node inspectors.
add_subdirectory(tutorials/reflection)

# Dynamic library loading and plugin entry points.
add_subdirectory(plugins/dynamic_library)
