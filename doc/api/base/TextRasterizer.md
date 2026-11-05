# TextRasterizer and TextLayout

**Header:** `<bg2e/base/TextRasterizer.hpp>` · **Namespace:** `bg2e::base`

Stateless CPU paragraph layout and RGBA rasterization with an explicitly resolved
immutable [TextFont](TextFont.md). No registry lookup, filesystem or GPU work.

```cpp
static TextLayout layout(const Text& text, const TextFont& font) noexcept;
static TextRasterization rasterize(const Text& text, const TextFont& font) noexcept;
```

Do not dereference a null font to call these methods. The owner can instead use
[TextTextureGenerator](TextTextureGenerator.md), which accepts a null font and
produces transparent output.

## Results

`TextLayout::Glyph` has `uint32_t index`, `double x, baseline` in logical units
from the paragraph's top-left. `Line` has `size_t firstGlyph, glyphCount` and
`double width, baseline`. The layout owns `glyphs`, `lines`, logical
`contentWidth/contentHeight`, `overflowed` (default false), and `renderable`
(default true). **A false renderable result is unavailable, not drawable ink.**

`TextRasterization` contains unsigned width/height, its `TextLayout`, and
`vector<uint8_t> rgba`. RGB is 255 even in fully transparent pixels; alpha is
glyph coverage composed source-over. Signed clipping precedes pixel indexing.
Empty content returns zero lines and transparent pixels; empty/whitespace text
must not accidentally become an opaque rectangle when a renderer sees it.

## Layout rules

Simple scalar LTR layout supports Latin/Spanish precomposed characters present in
the font. Uses native advances, kerning and glyph ink bounds, including negative
bearings/descenders. No full international shaping, bidi, grapheme-aware wrapping,
combining positioning, emoji/color fonts or automatic fallback chains.

- CR/LF/CRLF are explicit breaks; a trailing break preserves an empty final line.
- Tabs expand to four spaces, not tab stops. Other C0/C1 controls use U+FFFD.
- Word wrapping breaks at ASCII spaces/tabs and falls back to glyph boundaries
  for oversized words. Character/no-wrap modes are also available.
- Trailing breakable spaces are visually trimmed; explicit leading spaces are
  otherwise retained. Automatic breaks discard their separator.
- Horizontal/vertical alignment uses the logical margin interior. Font size is
  ascent-to-descent height. Baseline advance is
  `(ascent - descent + max(lineGap,0)) * logicalScale * lineSpacing`.
- Pixel densities never change logical measurements/wrap decisions. Both bitmap
  axes can have different densities. World-space size belongs to TextComponent.

## Bounded recovery

`MaxGlyphs = 65536` includes expanded tabs/newlines;
`MaxRasterizedGlyphPixels = 64*1024*1024` bounds glyph-mask work. Excessive layout
truncates with warnings. Overflow clips by default, or hides with warnings when
requested; legacy Error means Hide, not an exception. Raster/work/allocation
failure returns transparent/unavailable output instead of successful partial ink.
Check dimensions, pixel storage and `layout.renderable` before consuming a result;
allocation failure may leave an empty raster. No frame retries occur here.

See [Text data limits](Text.md), [cached generator](TextTextureGenerator.md), and
[TextComponent](../scene/TextComponent.md).
