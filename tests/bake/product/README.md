# Independent standalone product artifact checks

`inspect_output.py` reads an existing committed `lightmaps.json` without loading
the engine, initializing a GPU or changing files. It is independent of the C++
writer's own decode checks. Python 3.9+ and its standard library are sufficient.

```sh
python3 tests/bake/product/inspect_output.py /scratch/baked/lightmaps.json
python3 tests/bake/product/inspect_output.py /scratch/baked/lightmaps.json --expect /scratch/expected.json
```

Supported maps are the writer's noninterlaced 8-bit RGB/RGBA PNG and single-mip
Radiance RGBE HDR files. JPEG/BMP/TGA jobs require another independent decoder and
the lead-run engine probes; this inspector rejects unsupported formats instead
of reporting partial success. Native/PNG files and decoded PNG data are limited
to 64 MiB per file/map for bounded validation memory. Larger outputs need another
decoder; that limit does not constrain engine export capabilities.

Checks include:

- Unique nonempty target identities, one complete generation, local regular
  nonsymlink artifacts, path containment, no repeated declared paths and metadata
  identity/extent agreement. No directory scanning replaces manifest lookup.
- PNG chunk CRCs, dimensions, zlib termination and all five row filters; raw
  RGB bytes retain top-row order. Nonlinear PNG encoding declarations reject.
- Radiance HDR header, dimensions, flat/RLE RGBE scanlines and rejection of
  truncation/trailing bytes. RGBE quantization is expected; no alpha is stored.
- Linear UV2/multiplier/mode, conversion/exposure, sample/dilation metadata.
- For model copies, the BG2 v1.2 material section, local standard image sidecars,
  unit AO scale/channel zero, bakedAO/bakedGI/linear/UV2, and actual baked map RGB
  equality with the separate map file (including HDR above 65504).

The inspector deliberately does **not** parse/reload BG2 geometry, initialize the
renderer, prove lighting correctness, validate chart masks/dilation boundaries,
measure performance, verify source preservation or reconstruct cancelled jobs.
Native engine reload and known-scene comparisons remain mandatory lead checks.
It reads the on-disk material section, not a JSON substring guessed from binary.
JSON duplicate keys and nonfinite constants reject. Synthetic fixtures exercise
standard PNG and Radiance HDR decoding independently of engine execution.

The optional expectation file maps **actual output identities** to independently
chosen fixture expectations. For example, after obtaining identities from the
known test scene (do not copy pixel values from the output under test):

```json
{
  "targets": {
    "known-fixture-identity": {
      "metadata": {"accumulationFrames": 2, "uvSet": 1},
      "model": true,
      "tolerance": 0.000001,
      "maxAbove": 65504,
      "pixels": [
        {"x": 0, "y": 0, "rgb": [0, 100000, 2.5]},
        {"x": 1, "y": 1, "rgb": [7, 8, 9]}
      ]
    }
  }
}
```

These example pixels describe a controlled CPU writer fixture, **not** expected
lighting from an arbitrary scene. RGB8 probes use integer channels 0..255;
RGB32F uses linear floats. Exact target-set checks catch missing/skipped targets.
Known asymmetric corner pixels establish file orientation; chart-edge probes
must be chosen separately from a fixture's actual coverage/nearest-chart masks.

The report prints decoded ranges, dimensions, format, sidecar counts and image
SHA256. Retain it with native run records. It does not write a report over source
files; redirect stdout only to a deliberate external evidence destination.

## Synthetic inspector self-checks

```sh
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s tests/bake/product -p 'test_*.py' -v
```

Ten tests passed during M08-05 implementation. Inputs are temporary synthetic
files: typed HDR/PNG, all PNG filters/CRC, expected orientation, multiple targets,
material-section sidecars, corrupt payloads/metadata, duplicate keys, containment
and symlinks. A material-prefix-only fixture is intentionally **not** a complete
reloadable BG2 model. This result validates the inspector, not engine output,
backend behavior, native packaging or product acceptance. No C++/shader build,
engine application, native probe or package operation was run by the agent.

See [product validation record](../../../docs/render_to_draw/plan/milestone_08_lightmap_generator/product-validation.md)
for the lead's full parity/deployment/regression evidence matrix.
