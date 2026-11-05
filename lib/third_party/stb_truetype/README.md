# stb_truetype

Independently vendored `stb_truetype.h` v1.26, unmodified from:

- Repository: <https://github.com/nothings/stb>
- Revision: `6e9f34d5429cf16790ec43c9bac3f1ee4ad1f760`
- SHA-256: `ecd30b05e0dd4fea3a13c26810dd9e1992dc379049482c393d5a19e6b5090aab`
- Source: <https://raw.githubusercontent.com/nothings/stb/6e9f34d5429cf16790ec43c9bac3f1ee4ad1f760/stb_truetype.h>
- License: **MIT option** (the complete upstream dual-license notice is retained
  at the end of the header). This project selects the MIT option.
  `assets/licenses/stb_truetype-MIT.txt` supplies the notice in asset distributions.

Only `lib/src/bg2e/base/TextFont.cpp` defines `STB_TRUETYPE_IMPLEMENTATION` and
`STBTT_STATIC`. No ImGui stb implementation or public stb types are used.

**Trusted project fonts only.** Upstream explicitly states that this rasterizer
does not range-check every font offset and must not be used for untrusted font
files. Engine size/table checks are not a security sandbox or a font validator.
