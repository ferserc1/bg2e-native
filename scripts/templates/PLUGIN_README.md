# Application plugin

The generated application includes an empty native plugin target named
`<project-name>_plugin`. Its implementation is `src/ApplicationPlugin.cpp`.
The application target depends on this module and on its deployment target.
MainLoop discovers the resulting module automatically in the application's
`plugins` directory, inside `Contents/Resources` on macOS.

Add private component sources to the `SOURCES` list of `bg2e_add_plugin` in
`app/CMakeLists.txt`. Implement component behavior and reflection in those sources,
then call the supplied registrar from `ApplicationPlugin::registerComponents`.
Use the registrar for supporting reflection types and polymorphic subtypes too;
do not use static/global registration. Zero registered components is valid.
Choose an application-specific plugin ID and maintain its version/license metadata.

The module links the same precompiled shared engine as the application. Keep
`include/bg2e/plugin/BuildConfig.hpp`, `cmake/bg2e-sdk.cmake`, the public SDK headers
and the current-platform engine binary together. Use the engine's compatible
compiler, architecture, standard library and runtime settings. Debug/Release names
alone do not determine CRT compatibility; this SDK uses `MultiThreadedDLL` on MSVC.
CMake 3.23 or later is required. The shared helper is `cmake/plugins.cmake`.

`Application::preparePluginShutdown` already cleans the template's scene through
`Plugin::removeComponents`. If you add scenes, history, detached objects, weak
control blocks, subscriptions or background work, release those uses too before
unload. The programmer owns that contract: surviving plugin uses can crash after
the library closes. A scene cleanup call is not a lifetime census. Read the
[unload contract](../doc/api/plugin/unloading.md) and the shared
[CMake helper guide](../cmake/PLUGINS.md).
