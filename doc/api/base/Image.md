# Image

**Header:** `<bg2e/base/Image.hpp>`  
**Namespace:** `bg2e::base`

`Image` owns CPU pixel storage and metadata. It has no GPU allocation.

## Storage and ownership

Construct an empty image, or pass `unsigned char*` (SDR) or `float*` (HDR),
followed by width, height, and channel count. Both constructors and the matching
`setData` overloads take ownership of the supplied array. Allocate compatible
storage with `new[]`; the image releases it with `delete[]`.

`setData` first frees previous storage. `cleanup()` frees either buffer and
resets dimensions/channels to zero; the destructor calls it automatically.
Do not copy an owning `Image`: its implicit copy operations copy raw pointers
and do not provide independent pixel ownership.

| Member | Meaning |
|--------|---------|
| `data()` | Mutable/const byte buffer; null for HDR storage. |
| `dataf()` | Mutable/const float buffer; null for SDR storage. |
| `width()`, `height()`, `channels()` | Pixel dimensions and components per pixel. |
| `isValid()` | Currently tests the byte buffer and nonzero dimensions/channels; HDR images can return false. |
| `setPath(string)`, `path()` | File path or logical identifier metadata. |
| `setColorType(Color::Type)`, `colorType()` | Color-space metadata, initially `TypeSRGB`. |

Borrowed pixel pointers expire on replacement, cleanup, or destruction. Neither
path nor color-space metadata loads a file or converts its pixels.

## See also

- [Base API](index.md)
- [API reference](reference.md)
