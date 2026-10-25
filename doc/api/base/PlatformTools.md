# PlatformTools

**Header:** `<bg2e/base/PlatformTools.hpp>`  
**Namespace:** `bg2e::base`

`PlatformTools` provides static platform identification and filesystem paths.
Returned paths do not generally imply that a file or directory exists.

| Member | Result |
|--------|--------|
| `currentPlatform()` | `Platform::macOS`, `Platform::Windows`, or `Platform::Linux`. |
| `applicationPath()` | Executable directory on Linux/Windows; application bundle directory on macOS. Independent of working directory. |
| `shaderPath()` | `applicationPath()/shaders`, or `Contents/Resources/shaders` inside the macOS bundle. |
| `assetPath()` | Corresponding `assets` directory. |
| `pluginPath()` | Executable-sibling `plugins` on Linux/Windows; `Contents/Resources/plugins` in the macOS application bundle. Does not create the directory. |
| `settingsPath()` | Application settings location; requires an active `app::MainLoop` for its application ID. |
| `homePath()` | Current user's home directory. |

Linux executable discovery resolves `/proc/self/exe`; Windows uses
`GetModuleFileNameW`. Discovery failures throw. Linux settings use
`~/.config/<appId>`; Windows uses local application data plus `<appId>`.
For resource paths, append your resource's relative name to the returned base.

The header also supplies `BG2E_IS_MAC`, `BG2E_IS_WINDOWS`, or `BG2E_IS_LINUX`
and global `is_mac`, `is_windows`, `is_linux` constants for platform branches.

## See also

- [Base API](index.md)
- [API reference](reference.md)
- [Plugin discovery and lifecycle](../plugin/lifecycle.md)
- [Automatic plugin tutorial](../../tutorials/plugins/plugin.md)
