"""Synthetic CPU file-fixture tests; these never launch the engine or its CLI."""
import math
import json
from pathlib import Path
import struct
import tempfile
import unittest
import zlib

import inspect_output as check


def chunk(kind, payload):
    return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", zlib.crc32(kind + payload))


def png_bytes(width, height, rgb, filters=None):
    stride = width * 3
    rows, previous = bytearray(), bytearray(stride)
    for y in range(height):
        row = bytearray(rgb[y * stride:(y + 1) * stride])
        mode = filters[y] if filters else 0
        encoded = bytearray()
        for x, value in enumerate(row):
            a = row[x - 3] if x >= 3 else 0
            b = previous[x]
            c = previous[x - 3] if x >= 3 else 0
            predictor = (0, a, b, (a + b) // 2, check.paeth(a, b, c))[mode]
            encoded.append((value - predictor) & 255)
        rows.extend(bytes([mode]) + encoded)
        previous = row
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">2I5B", width, height, 8, 2, 0, 0, 0)) +
            chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b""))


def float_archive(rgb):
    result = bytearray(b"#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y 2 +X 2\n")
    for i in range(0, len(rgb), 3):
        pixel = rgb[i:i + 3]
        maximum = max(pixel)
        if maximum < 1e-32:
            result.extend(bytes(4))
        else:
            mantissa, exponent = math.frexp(maximum)
            scale = mantissa * 256 / maximum
            result.extend(bytes([int(v * scale) for v in pixel] + [exponent + 128]))
    return bytes(result)


class InspectionTests(unittest.TestCase):
    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory()
        self.addCleanup(self.scratch.cleanup)
        self.root = Path(self.scratch.name)
        self.generation = self.root / "lightmaps-synthetic"
        self.generation.mkdir()
        self.rgb = [0.0, 100000.0, 2.5, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0]
        self.image = self.generation / "map.hdr"
        self.image.write_bytes(float_archive(self.rgb))
        self.metadata = {"version": 1, "identity": "mesh/0", "image": "map.hdr", "width": 2, "height": 2,
                         "colorSpace": "linear", "rowZero": "v=0", "uvSet": 1, "signal": "indirectMultiplierRGB",
                         "padding": "nearest-chart", "indirectTextureMode": "bakedGI", "dilationPixels": 1,
                         "accumulationFrames": 2, "samplesPerPixel": 8, "cpuFormat": "RGB32F",
                         "storageFormat": "Radiance-RGBE", "exposureApplied": False,
                         "conversion": "Radiance-RGBE-quantization", "byteEncodingLossy": False}
        self.meta = self.generation / "map.lightmap.json"
        self.save_metadata()
        self.document = {"version": 1, "targets": [{"identity": "mesh/0", "image": "lightmaps-synthetic/map.hdr",
                                                   "metadata": "lightmaps-synthetic/map.lightmap.json"}]}
        self.manifest = self.root / "lightmaps.json"
        self.save_manifest()

    def save_metadata(self):
        self.meta.write_text(json.dumps(self.metadata), encoding="utf-8")

    def save_manifest(self):
        self.manifest.write_text(json.dumps(self.document), encoding="utf-8")

    def inspect(self, expect=None):
        return check.inspect(self.manifest, expect)

    def test_hdr_and_independent_orientation_expectations(self):
        report = self.inspect({"targets": {"mesh/0": {"maxAbove": 65504, "model": False,
            "metadata": {"accumulationFrames": 2}, "pixels": [{"x": 0, "y": 0, "rgb": [0, 99840, 0]},
                                                            {"x": 1, "y": 1, "rgb": [7, 8, 9]}]}}})
        self.assertGreater(report["targets"][0]["maximum"], 99000)
        self.assertEqual(len(report["targets"][0]["imageSHA256"]), 64)
        with self.assertRaises(check.InvalidOutput):
            self.inspect({"targets": {"mesh/0": {"pixels": [{"x": 0, "y": 0, "rgb": [4, 5, 6]}]}}})

    def test_png_all_filters_and_crc(self):
        rgb = bytes((i * 31) % 256 for i in range(2 * 5 * 3))
        path = self.generation / "filters.png"
        path.write_bytes(png_bytes(2, 5, rgb, [0, 1, 2, 3, 4]))
        self.assertEqual(check.png_rgb(path), (2, 5, rgb))
        broken = bytearray(path.read_bytes())
        broken[-1] ^= 1
        path.write_bytes(broken)
        with self.assertRaisesRegex(check.InvalidOutput, "CRC"):
            check.png_rgb(path)

    def test_byte_map(self):
        rgb = bytes(range(12))
        image = self.generation / "map.png"
        image.write_bytes(png_bytes(2, 2, rgb))
        self.document["targets"][0]["image"] = "lightmaps-synthetic/map.png"
        self.metadata.update(image="map.png", cpuFormat="RGB8", storageFormat="RGB8",
                             exposureApplied=True, conversion="linear-exp2-exposure-clamp-round-unorm8")
        self.save_metadata()
        self.save_manifest()
        self.assertEqual(self.inspect()["targets"][0]["maximum"], 11)

    def test_multiple_targets_and_mixed_generation_rejection(self):
        second_image = self.generation / "second.hdr"
        second_image.write_bytes(self.image.read_bytes())
        second_meta = dict(self.metadata, identity="mesh/1", image="second.hdr")
        (self.generation / "second.json").write_text(json.dumps(second_meta), encoding="utf-8")
        self.document["targets"].append({"identity": "mesh/1", "image": "lightmaps-synthetic/second.hdr",
                                        "metadata": "lightmaps-synthetic/second.json"})
        self.save_manifest()
        self.assertEqual(len(self.inspect()["targets"]), 2)
        other = self.root / "lightmaps-other"
        other.mkdir()
        (other / "second.hdr").write_bytes(second_image.read_bytes())
        self.document["targets"][1]["image"] = "lightmaps-other/second.hdr"
        self.save_manifest()
        with self.assertRaisesRegex(check.InvalidOutput, "mixes"):
            self.inspect()

    def test_material_prefix_sidecars_and_hdr(self):
        model_dir = self.generation / "mesh-model"
        model_dir.mkdir()
        (model_dir / "baked.hdr").write_bytes(self.image.read_bytes())
        material = {"indirectTextureMode": "bakedGI", "ambientOcclussionUV": 1,
                    "ambientOcclussionScale": [1, 1], "ambientOcclussionChannel": 0,
                    "ambientOcclussionSettings": {"color": 2}, "ambientOcclussion": "baked.hdr"}
        model = model_dir / "model.bg2"
        def save():
            encoded = json.dumps([material]).encode()
            # Deliberately material-section-only: this is NOT a reloadable geometry fixture.
            model.write_bytes(b"\x00\x01\x02\x00hedr" + struct.pack(">I", 1) + b"mtrl" +
                              struct.pack(">I", len(encoded)) + encoded)
        save()
        self.document["targets"][0]["model"] = "lightmaps-synthetic/mesh-model/model.bg2"
        self.save_manifest()
        self.assertEqual(self.inspect()["targets"][0]["modelSidecarsChecked"], 1)
        material["ambientOcclussionSettings"]["color"] = 1
        save()
        with self.assertRaisesRegex(check.InvalidOutput, "semantics"):
            self.inspect()
        material["ambientOcclussionSettings"]["color"] = 2
        save()
        changed = self.rgb.copy()
        changed[1] = 65504
        (model_dir / "baked.hdr").write_bytes(float_archive(changed))
        with self.assertRaisesRegex(check.InvalidOutput, "pixels differ"):
            self.inspect()

    def test_invalid_float_payloads(self):
        original = self.image.read_bytes()
        for data in (original[:-1], original + b"extra", original.replace(b"-Y 2 +X 2", b"-Y 3 +X 2")):
            self.image.write_bytes(data)
            with self.assertRaises(check.InvalidOutput):
                self.inspect()

    def test_metadata_identity_extent_and_conversion(self):
        for field, value in (("identity", "wrong"), ("height", 3), ("uvSet", 0),
                             ("colorSpace", "srgb"), ("conversion", "clamp"), ("exposureApplied", True)):
            original = self.metadata.copy()
            self.metadata[field] = value
            self.save_metadata()
            with self.assertRaises(check.InvalidOutput):
                self.inspect()
            self.metadata = original

    def test_duplicate_paths_identities_and_json(self):
        self.document["targets"].append(self.document["targets"][0].copy())
        self.save_manifest()
        with self.assertRaisesRegex(check.InvalidOutput, "Duplicate"):
            self.inspect()
        self.manifest.write_text('{"version":1,"version":1,"targets":[]}', encoding="utf-8")
        with self.assertRaisesRegex(check.InvalidOutput, "Duplicate JSON"):
            self.inspect()

    def test_missing_symlink_and_escape(self):
        for value in ("../map.hdr", "/absolute.hdr", "C:/map.hdr", "missing.hdr"):
            self.document["targets"][0]["image"] = value
            self.save_manifest()
            with self.assertRaises(check.InvalidOutput):
                self.inspect()
        link = self.generation / "alias.hdr"
        try:
            link.symlink_to(self.image.name)
        except OSError:
            self.skipTest("Host cannot create symlinks")
        self.document["targets"][0]["image"] = "lightmaps-synthetic/alias.hdr"
        self.save_manifest()
        with self.assertRaisesRegex(check.InvalidOutput, "Symlink"):
            self.inspect()

    def test_native_format_ordinals_match_source(self):
        source = (Path(__file__).resolve().parents[3] / "lib/include/bg2e/gpu/Common.hpp").read_text()
        enum = source.split("enum class PixelFormat", 1)[1].split("};", 1)[0]
        import re
        enum = re.sub(r"//[^\n]*", "", enum).split("{", 1)[1]
        names = [part.strip().split("=", 1)[0].strip() for part in enum.split(",") if part.strip()]
        self.assertEqual(names.index("R32G32B32A32_SFLOAT"), 6)
        self.assertEqual(names.index("R32G32B32A32_UINT"), 20)


if __name__ == "__main__":
    unittest.main()
