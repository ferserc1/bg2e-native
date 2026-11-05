# TextFont

**Header:** `<bg2e/base/TextFont.hpp>` · **Namespace:** `bg2e::base`

Immutable CPU resource owning loaded font bytes and private independently
vendored stb_truetype state. No public stb/ImGui types are exposed. No GPU or
scene dependency; the [scene registry](../scene/Scene.md#text-font-registry)
normally owns these resources for TextComponent.

```cpp
static std::shared_ptr<const TextFont> load(
    const std::filesystem::path& path, uint32_t faceIndex = 0) noexcept;
~TextFont();
TextFont(const TextFont&) = delete;
TextFont& operator=(const TextFont&) = delete;

const std::filesystem::path& path() const;
uint32_t faceIndex() const;
size_t byteSize() const;
Metrics metrics() const;
uint32_t glyphIndex(char32_t codepoint) const noexcept;
GlyphMetrics glyphMetrics(uint32_t glyph) const noexcept;
int kerningAdvance(uint32_t leftGlyph, uint32_t rightGlyph) const noexcept;
float scaleForPixelHeight(float pixelHeight) const noexcept;
GlyphBitmap rasterizeGlyph(uint32_t glyph, float pixelHeight,
    float shiftX = 0, float shiftY = 0) const noexcept;
GlyphBitmap rasterizeGlyphScaled(uint32_t glyph, float scaleX, float scaleY,
    float shiftX = 0, float shiftY = 0) const noexcept;
```

`Metrics` contains unscaled integer `ascent`, `descent`, `lineGap` and unsigned
`glyphCount`. `GlyphMetrics` contains integer `advanceWidth`, `leftSideBearing`
and bounds `x0,y0,x1,y1`. Kerning is in font units. Isotropic pixel height spans
ascent to descent; independent scales are pixels per unscaled font unit. Shifts
are subpixel offsets, not paragraph margins.

`GlyphBitmap` owns integer width/height/xOffset/yOffset and a row-major,
top-to-bottom `vector<uint8_t> coverage`. **Check `valid`**: false means raster
failure; valid zero-sized whitespace is not a failure. No borrowed stb buffer
escapes. Scalar lookup returns glyph 0 (.notdef) for missing characters, with no
font fallback/shaping/bidi.

## Failures and lifetime

`MaxFileBytes = 64*1024*1024`, `MaxFaces = 32`, and
`MaxGlyphBitmapDimension = 4096`. Unreadable/missing/unsupported/invalid files and
allocation failures warn and return null. An invalid face index recovers to face
0 when possible. Invalid Unicode/glyph indices use replacement/.notdef; unsafe
pixel scales/offsets use defaults/safe limits. Check returned resources before
querying them; `load()` does not create a dummy font or throw for normal font
errors. No watcher, network loader, system discovery or retry loop is installed.

Shared immutable handles keep their byte/parser storage alive independently of
registry removal/reload; they are snapshots, not live subscriptions. Serialize
loading, rasterization and text generator/cache use at the application's resource
boundary; the API does not promise concurrent parser/raster execution.

## Security and licensing

**Trusted project assets only.** Structural checks and C++ exception containment
do not sandbox stb against malicious native input/faults. Do not accept arbitrary
downloaded/user-uploaded fonts as safe merely because `load()` returns a status.

The rasterizer uses stb_truetype's **MIT option**, independent of ImGui's private
stb copy; implementation symbols are translation-unit private. See the pinned
[vendor provenance](../../../lib/third_party/stb_truetype/README.md) and shipped
[MIT notice](../../../assets/licenses/stb_truetype-MIT.txt). Font rights are
separate: canonical [Didact Gothic](../../../assets/fonts/didact-gothic/README.md)
is distributed under its full [OFL 1.1](../../../assets/fonts/didact-gothic/OFL.txt),
not under stb's license or the engine GPL. Preserve its copyright/license notice
and provenance; other fonts require their own redistribution rights/notices.

See [Text](Text.md), [TextRasterizer](TextRasterizer.md), and
[runnable tutorial](../../tutorials/text/README.md).
