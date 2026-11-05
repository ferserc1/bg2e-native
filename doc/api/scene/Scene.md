# Scene text resources

**Header:** `<bg2e/scene/Scene.hpp>` · **Namespace:** `bg2e::scene`

For graph/camera/environment ownership see the [scene index](index.md). This page
documents Scene's CPU font registry used by [TextComponent](TextComponent.md).

## Text font registry

```cpp
void prepareTextFonts() noexcept;
void reloadTextFonts() noexcept;
void invalidateTextFonts() noexcept;
std::shared_ptr<const base::TextFont> textFont(const std::string& identifier) const noexcept;
bool textFontsReady() const;
uint64_t textFontVersion() const;
```

`setSceneRoot(sharedRoot)` establishes registry ownership. `prepareTextFonts()`
first gathers every TextFontComponent, including disabled nodes/later siblings,
then loads/fonts privately and publishes complete availability outcomes. All
declarations share one namespace; duplicate identifiers across collections are
disabled with diagnostics naming their owners. Limits are 128 declarations and
128 MiB of loaded source bytes per scene. Unaffected fonts remain usable.

Unchanged path/face declarations reuse immutable resources. Failed sources stay
unavailable until an authored change or explicit reload; there is no per-frame
file retry/watcher. `invalidateTextFonts()` marks the registry dirty;
`reloadTextFonts()` invalidates and immediately prepares with fresh file reads.
Graph/declaration membership edits invalidate automatically; content-only Text
edits do not reload fonts. Root release/replacement clears registry ownership.

**Ready means preparation completed, not that every font loaded.** Lookup on a
dirty/missing/ambiguous/failed entry warns (deduplicated per generation) and returns
null, never a stale resource or an implicit default. Check the handle before use.
Missing/unsupported fonts and contained I/O/allocation failures do not throw out
of these registry boundaries or discard other scene content.

Previously returned handles are immutable retained snapshots. Consumers must
track both Scene identity and textFontVersion, not only a coincident generation
number after scene transfer/undo. TextComponent performs that bookkeeping.

Scene registry mutation/loading and text realization belong to the stopped
application resource boundary; do not access a mutating registry concurrently.
`Node::loadResources(engine, progress)` prepares the owning scene's fonts before
the first component. Detached font-bearing subtrees need an owning/temporary
Scene. Schema-only `db::loadScene(path)` loads neither fonts nor GPU resources;
its engine overload performs the font-first realization pass afterward.

See [tutorial loading order](../../tutorials/text/README.md#7-register-and-load-the-font),
[TextFontComponent](TextFontComponent.md), and [resource IO](../db/DrawResourceIO.md).
