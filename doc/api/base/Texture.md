# Texture and ProceduralTextureGenerator

**Header:** `<bg2e/base/Texture.hpp>`  
**Namespace:** `bg2e::base`

`Texture` describes a texture source and sampling preferences on the CPU. A
filesystem constructor stores a path; it does not load pixels or allocate a GPU
image. `ProceduralTextureGenerator` is the abstract source for generated pixels.

## Texture source

Construct `Texture` empty, from `std::filesystem::path`, `std::string`, a
path-plus-filename pair, or a raw/shared generator pointer. Raw generator
pointers transfer ownership to `shared_ptr`; shared pointers share ownership.
`type()` returns `TypeFilesystem` or `TypeProcedural`.

`setImageFilePath` selects a filesystem source. `setProceduralGenerator` selects
a procedural source. Read them with `imageFilePath()` and
`proceduralTextureGenerator()` (a borrowed pointer).

## Sampling and color settings

| Setters / getters | Values and defaults |
|-------------------|---------------------|
| `setUseMipmaps` / `useMipmaps` | Initially true. |
| `setMagFilter` / `magFilter` | `FilterLinear` by default. |
| `setMinFilter` / `minFilter` | `FilterNearest` by default. |
| `setMipLodBias` / `mipLodBias` | Initially 0. |
| `setMinLod` / `minLod` | Initially 0. |
| `setMaxLod` / `maxLod` | Initially -1, meaning the maximum available mip count. |
| `setAddressModeU/V/W` / `addressModeU/V/W` | Initially `AddressModeRepeat`. |
| `setColorType` / `colorType` | Initially `Color::TypeSRGB`. |

Address modes also include `AddressModeMirroredRepeat`, `AddressModeClampToEdge`,
and `AddressModeClampToBorder`. `setAddressMode(mode)` changes U and V;
`setAddressMode(u, v)` changes those axes; the three-argument overload changes
U, V, and W. Filters also include `FilterNearest` and `FilterLinear`.

`revision()` changes when authored settings change and observes generator
revisions. Copying shares generator ownership; assignment updates settings
through revision tracking.

## ProceduralTextureGenerator

Subclass it and implement `uint8_t* generate()` and `std::string imageIdentifier()`.
The first produces an owned byte array compatible with `Image`'s `delete[]`;
the second identifies equivalent content for texture caching. `colorType()`
is virtual and defaults to `TypeLinear`.

Construct with dimensions and bytes per pixel, or set them with `setDimensions`,
`setWidth`, `setHeight`, and `setBytesPerPixel`. Corresponding getters expose
the values. Dimension changes increment `revision()`; call `markModified()`
when custom pixel-producing state changes.

`Texture::generateImage()` requires a generator or throws `std::runtime_error`.
It returns a newly allocated `Image*` owned by the caller, transfers the generated
buffer to that image, sets its identifier to `procedural-texture:` plus
`imageIdentifier()`, and copies the generator's color type to the image/texture.
Use `std::unique_ptr<Image>` to own the returned image.

## See also

- [Base API](index.md)
- [API reference](reference.md)
