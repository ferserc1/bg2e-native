# ImageLoadContext

**Header:** `<bg2e/draw/ImageLoadContext.hpp>`  
**Namespace:** `bg2e::draw`

Short-lived, single-threaded resource-loading context for one initialized Engine.
It reuses decoded CPU pixels and compatible GPU images while keeping every
returned `Texture`, its editable settings, and its sampler independent.

```cpp
bg2e::draw::ImageLoadContext context(&engine);
auto first = context.load(firstSettings);
auto second = context.load(secondSettings);
```

Include the context header explicitly. The Engine/device must outlive the context,
returned textures, and all GPU users. Stop recording/background producers before
loading, as with other synchronous resource operations.

## API

| Operation | Contract |
| --- | --- |
| `ImageLoadContext(Engine*)` | Binds the context to a non-null, initialized Engine. Copying is disabled. |
| `engine()` | Returns the borrowed Engine pointer. |
| `registerPixels(path, shared_ptr<const base::Image>)` | Seeds a file-backed source with decoded RGBA byte or float pixels; the nonempty path remains its reload/save source. Register once, before loading that source. Duplicate registration is rejected. |
| `load(const base::Texture&)` | Returns a new texture/settings/sampler owner, reusing compatible image storage. Procedural sources bypass reuse. |
| `neutralTexture(bool normal)` | Returns an independent texture using the context's shared white or flat-normal image. Empty-path reload remains supported through retained one-pixel CPU data. |

Registered pixel buffers must not be modified through other aliases while the
context uses them. Registration does not write the source file; the caller is
responsible for its existence and lifetime if reload/save will use it.

## Image identity and lifetime

CPU pixels are keyed by absolute, lexically normalized path. GPU image identity
adds mipmap enablement and color interpretation. Sampler filters, addressing, and
LOD settings are not image identity: each returned texture creates its own sampler.
Different mipmap/color requirements may therefore upload separate images while
still reusing the decoded CPU source.

There is no content hashing, timestamp invalidation, symbolic-link canonicalization,
global cache, or cross-device sharing. One context represents a load-time snapshot
of each source. Use a new context to observe changed files.

The context strongly retains CPU pixels and GPU image entries until destruction.
Keep its scope bounded to an import; returned textures retain their GPU images
afterward. File-backed reloads do not retain the context and read their source
normally. Reload/load replace a texture's image rather than modifying a shared
image in place. Do not mutate shared GPU storage through raw image accessors;
deep-clone first when independent writable pixels are needed.

## Drawable and material integration

Existing calls remain available and create a local context:

```cpp
drawable.load(&engine, onTextureLoaded);
drawable.updateMaterials(onTextureLoaded);
material.updateTextures(onTextureLoaded);
```

To share images across several Drawables, pass one explicit context:

```cpp
bg2e::draw::ImageLoadContext context(&engine);
firstDrawable.load(&engine, context, onTextureLoaded);
secondDrawable.load(&engine, context, onTextureLoaded);
```

Other overloads are `Drawable::updateMaterials(context, callback)` and
`MaterialBase::updateTextures(context, callback)`. Context and resource must belong
to the same Engine. Callbacks retain their per-assigned-slot behavior, including
when an image is reused; they are not counts of physical uploads.

With `MaterialBase::useTextureCache()` explicitly enabled, file sources still use
the existing `utils::TextureCache` semantics instead. That cache shares whole
texture objects; the import context shares only image storage. Procedural sources
are not deduplicated by either mechanism.

BG2 loading benefits from the Drawable's local context. glTF/GLB loading shares a
context across imported Drawables, seeds embedded-image pixels after writing their
reload/export PNG sources, and leaves external image paths intact.

Uploads and mip generation remain synchronous and per image. Batched uploads,
staging pools, and reduced submission waits are future work, not part of this API.

See [resource ownership](Resources.md), [complete resource IO](../db/DrawResourceIO.md),
and [image loading architecture](../../architecture/texture_and_image_load.md).
