# Didact Gothic Regular — canonical text font

`DidactGothic-Regular.ttf` is the unmodified static font from Google Fonts:

- Repository: <https://github.com/google/fonts>
- Pinned revision: `c741144416d5635d5de832dff8ba09292c112036`
- Font: <https://raw.githubusercontent.com/google/fonts/c741144416d5635d5de832dff8ba09292c112036/ofl/didactgothic/DidactGothic-Regular.ttf>
- License: <https://raw.githubusercontent.com/google/fonts/c741144416d5635d5de832dff8ba09292c112036/ofl/didactgothic/OFL.txt>
- Font SHA-256: `b98f9e091b6337a86f4e0f69c94c31905e1e287782853f472e176b4e28d3f81f`
- License: **SIL Open Font License 1.1**, reproduced in full in `OFL.txt`.
- Original notice: Copyright (c) 2009, 2010, 2011 Daniel Johnson.

Use this path for text-component examples and the [text tutorial](../../../examples/tutorials/text/README.md):
`fonts/didact-gothic/DidactGothic-Regular.ttf`, relative to the engine assets root.
The asset-tree copy/install rules include the font, this record and its license.

The pre-existing `assets/DidactGothic-Regular.ttf` was introduced in repository
commit `2c384266`. Its SHA-256 is
`b41981e61095049122f77e334758fa635d11195814e1130efe4b7d810445d741`.
It does not match the pinned upstream original; its exact upstream provenance
was not established. It is intentionally preserved, not replaced or used as the
canonical text asset. This separate verified copy is necessary for reproducible
licensing/provenance, rather than an unexplained duplicate.

Only trusted project fonts are supported. stb_truetype's MIT license covers the
rasterizer, **not** this font, and its parser is not safe for untrusted input.
