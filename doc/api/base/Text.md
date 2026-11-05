# Text

**Header:** `<bg2e/base/Text.hpp>` · **Namespace:** `bg2e::base`

CPU UTF-8 paragraph data, with no font file handle, scene, GPU resource, color or
material. Use [TextComponent](../scene/TextComponent.md) to draw it, or supply a
resolved [TextFont](TextFont.md) to [TextRasterizer](TextRasterizer.md).

## Values and defaults

`Text()`, `explicit Text(Parameters)`, copy construction,
`Text& operator=(const Text&) noexcept` and `bool operator==(const Text&) const`
operate on independent values. Assignment compares parameters rather than
importing the source's revision.

| `Parameters` field | Default | Unit / interpretation |
|---|---|---|
| `std::string content` | empty | Normalized UTF-8, maximum 64 KiB. |
| `std::string fontIdentifier` | `default` | Explicit scene identifier, not an automatic fallback. |
| `float fontSize` | 32 | Logical ascent-to-descent height. |
| `float lineSpacing` | 1.2 | Multiplier of native line advance, including nonnegative font line gap. |
| `float characterSpacing` | 0 | Additional logical advance between adjacent glyphs. |
| `Size paragraphSize` | `{512,256}` | Logical width/height; independent of world size. |
| `Resolution bitmapResolution` | `{1024,512}` | Unsigned pixel width/height; independent X/Y densities. |
| `Margins margins` | `{8,8,8,8}` | Logical left/top/right/bottom inset. |
| `Alignment alignment` | `Left` | `Left`, `Center`, `Right`. |
| `VerticalAlignment verticalAlignment` | `Top` | `Top`, `Center`, `Bottom`. |
| `WrapMode wrapMode` | `Word` | `None`, `Word`, `Character`. |
| `OverflowPolicy overflowPolicy` | `Clip` | `Clip` or `Hide`; legacy `Error` aliases `Hide`. |

`Size`, `Resolution`, `Margins` and `Parameters` have value equality. Geometry
does not auto-expand to fit content. Bitmap resolution changes sampling quality,
not logical line breaks, font measurements or the component's world dimensions.

## Editing API

```cpp
const Parameters& parameters() const;
void setParameters(Parameters parameters) noexcept;
uint64_t revision() const;

const std::string& content() const;
void setContent(std::string value);
const std::string& fontIdentifier() const;
void setFontIdentifier(std::string value);
float fontSize() const;             void setFontSize(float value);
float lineSpacing() const;          void setLineSpacing(float value);
float characterSpacing() const;     void setCharacterSpacing(float value);
Size paragraphSize() const;         void setParagraphSize(Size value);
Resolution bitmapResolution() const; void setBitmapResolution(Resolution value);
Margins margins() const;            void setMargins(Margins value);
Alignment alignment() const;        void setAlignment(Alignment value);
VerticalAlignment verticalAlignment() const;
void setVerticalAlignment(VerticalAlignment value);
WrapMode wrapMode() const;          void setWrapMode(WrapMode value);
OverflowPolicy overflowPolicy() const;
void setOverflowPolicy(OverflowPolicy value);
```

Use atomic `Parameters` replacement for coupled box/margin/resolution edits. A
no-op preserves the revision; an effective normalized edit increments it. These
are CPU mutations, not thread-safe asynchronous requests. Serialize edits on the
owning application thread; see the [tutorial](../../tutorials/text/README.md) for
queued runtime edits.

## Validation and recovery

Invalid values warn and recover, not throw as ordinary validation control flow:

- Identifiers accept 1–128 ASCII letters/digits or `_`, `-`, `.`, `:`. Invalid
  identifiers become `default`, which must still be declared explicitly.
- Nonpositive/nonfinite logical dimensions use 512/256 defaults; valid positive
  requests clamp to `[1,1000000]`. Font height clamps to `[1,4096]`, line spacing
  to `[0.1,10]`, character spacing to `[-4096,4096]`, with defaults for nonfinite
  values/nonpositive height or spacing.
- Margins are nonnegative, bounded to 1000000; an axis consumed by its margins
  gets zero margins on that axis, preserving a positive interior.
- Bitmap axes are `[1,8192]`, at most 16 Mi pixels (64 MiB RGBA). Zero axes use
  defaults; excessive allocation or glyph-scale density reduces resolution
  without changing logical layout. Font bitmap safety limits still apply.
- Invalid UTF-8 bytes normalize to U+FFFD; oversized content truncates at a scalar
  boundary. Unknown enums recover to their defaults. Allocation failure inside an
  edit warns and retains the previous value. Argument construction/copying can
  allocate before a method is entered; callers still contain failures at their
  application mutation boundary.

Layout/raster recovery and supported scripts are documented under
[TextRasterizer](TextRasterizer.md); full shaping, bidi, fallback chains,
combining-mark placement, rich text and SDF/MSDF are not implemented.

## JSON

Matches Light's representation-only contract:

```cpp
void deserialize(std::shared_ptr<json::JsonNode> data) noexcept;
std::shared_ptr<json::JsonNode> serialize() noexcept;
```

Keys match the fields above; size/resolution are `{width,height}`, margins are
`{left,top,right,bottom}`, and enums use lowercase strings (`left`, `center`,
`right`; `top`, `center`, `bottom`; `none`, `word`, `character`; `clip`, `hide`).
Legacy overflow `error` reads as hide-and-warn. Missing/wrongly typed optional
fields retain existing values. Parsing loads no files and allocates no GPU
resources. A failed serializer warns and returns null; save owners must check it.

See [font declarations](../scene/TextFontComponent.md),
[scene registry](../scene/Scene.md#text-font-registry) and [Base index](index.md).
