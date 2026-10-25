# Raw dynamic-library example

`dylib_example_app` displays a fixed background and a **Load library** button.
The button uses `base::DynamicLibrary` to open `dylib_example_lib`, resolve
`getMessage` and display a host-owned copy of its message.

This library is not an engine Plugin. MainLoop's automatic scan reports
`NotAPlugin` and continues; the button's explicit raw-library load remains
independent. The target intentionally does not use `bg2e_add_plugin`.

Linux/Windows runtime output is isolated under
`bin/<platform>/examples/dynamic_library` (with configuration subdirectories on
Windows). The existing application helper deploys engine runtime dependencies,
shaders and assets at that destination. macOS retains its normal app bundle.
`PlatformTools::pluginPath()` matches the private deployment directory on each
platform; library output filenames come from CMake target expressions.

See the [complete tutorial](../../../doc/tutorials/plugins/dynamic_library.md),
including the [manual migration of historical shared copies](../../../doc/tutorials/plugins/dynamic_library.md#remove-the-historical-shared-copy-after-migration).
Only the exact old `libdylib_example_lib.so` or `dylib_example_lib.dll` file should
be removed when present. No files are automatically deleted by configuration.
