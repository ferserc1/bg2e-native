# GI bake filter reference checks

Run from the repository root:

```sh
python3 tests/bake/filter/reference.py
```

The CPU model checks noise reduction on a deterministic planar signal, constant
HDR preservation including atlas edges, chart and geometry discontinuity
isolation, uncovered texels and singleton charts. A source parity check ensures
the GLSL and Metal algorithm bodies match. These checks do not compile shaders
or establish native binding correctness or visual quality on a GPU.

For native verification, bake the same target with fixed resolution, eight
samples per pixel and three bounces at multiple accumulation counts. Inspect both
the completed preview and exported lightmap, including adjacent charts and contact
shadows. The final filter must affect both outputs while raw sample counters
continue to reach the requested accumulation count. AO must remain unchanged.
