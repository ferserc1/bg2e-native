# TextureCache

**Header:** `<bg2e/utils/TextureCache.hpp>` · **Namespace:** `bg2e::utils`.

`TextureCache::get()` returns the shared cache. `load(draw::Engine*, path)`, the
base-path/file-name and string variants, and the base::Texture-settings overload
return draw::Texture owners. Production overloads and strong production maps are
removed.

Entries are weak and keyed by context plus texture settings. Reuse checks Engine,
live image/device and current texture settings; expired or incompatible entries
are replaced. External owners retain resource lifetime. Engine/device must outlive
all returned resources. `emptyCache()` drops weak entries; `destroy()` releases the
singleton. Coordinate cache/resource changes with stopped producers.

Caching is explicit sharing, not a replacement for independent Drawable cloning.
Material/texture revisions and native image generations still govern realized
bindings. See [draw resource ownership](../draw/Resources.md).

This whole-texture cache is separate from [ImageLoadContext](../draw/ImageLoadContext.md),
which ordinary material/Drawable loading uses to share only decoded pixels and
GPU images within an operation. That context keeps texture settings and samplers
independent and does not change this singleton's weak-entry semantics. Neither
mechanism hashes file contents or automatically detects file edits. Procedural
sources remain excluded from reuse.
