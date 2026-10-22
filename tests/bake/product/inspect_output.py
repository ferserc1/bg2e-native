"""Read-only M08 product artifact inspection; no engine/GPU dependency.

Supports the writer's PNG and Radiance HDR maps. BG2 inspection covers its
material section and referenced standard images, not geometry or engine reload.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path, PurePosixPath
import struct
import zlib


class InvalidOutput(ValueError):
    pass


def require(condition, message):
    if not condition:
        raise InvalidOutput(message)


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, f"Duplicate JSON key: {key}")
        result[key] = value
    return result


def parse_json(data):
    def invalid_constant(value):
        raise InvalidOutput(f"Nonfinite JSON constant: {value}")
    return json.loads(data, object_pairs_hook=unique_object, parse_constant=invalid_constant)


def artifact(root, relative):
    require(isinstance(relative, str) and relative and "\\" not in relative and ":" not in relative,
            "Artifact needs a portable relative path")
    parts = PurePosixPath(relative).parts
    require(not PurePosixPath(relative).is_absolute() and all(p not in (".", "..") for p in parts),
            "Artifact path escapes generation")
    path = root
    for part in parts:
        path /= part
        require(not path.is_symlink(), f"Symlink artifact: {relative}")
    require(path.is_file() and path.resolve().is_relative_to(root.resolve()), f"Missing/outside artifact: {relative}")
    return path


def hdr_rgb(path):
    require(path.stat().st_size <= 64 * 1024 * 1024, "HDR exceeds file limit")
    with path.open("rb") as stream:
        require(stream.readline().strip() in (b"#?RADIANCE", b"#?RGBE"), "Invalid HDR signature")
        while True:
            line = stream.readline()
            require(line, "Truncated HDR header")
            if not line.strip():
                break
        resolution = stream.readline().split()
        require(len(resolution) == 4 and resolution[0] == b"-Y" and resolution[2] == b"+X", "Unsupported HDR orientation")
        height, width = int(resolution[1]), int(resolution[3])
        require(width > 0 and height > 0 and width * height <= 4 * 1024 * 1024, "Invalid HDR extent")
        rgb = []
        for _ in range(height):
            first = stream.read(4)
            require(len(first) == 4, "Truncated HDR row")
            if 8 <= width < 32768 and first[:2] == b"\x02\x02":
                require(int.from_bytes(first[2:], "big") == width, "Invalid HDR scanline")
                channels = []
                for _ in range(4):
                    channel = bytearray()
                    while len(channel) < width:
                        code = stream.read(1)
                        require(code and code[0] != 0, "Invalid HDR run")
                        count = code[0] - 128 if code[0] > 128 else code[0]
                        payload = stream.read(1 if code[0] > 128 else count)
                        require(len(payload) == (1 if code[0] > 128 else count), "Truncated HDR run")
                        channel.extend(payload * count if code[0] > 128 else payload)
                        require(len(channel) <= width, "HDR run exceeds row")
                    channels.append(channel)
                row = zip(*channels)
            else:
                data = first + stream.read(width * 4 - 4)
                require(len(data) == width * 4, "Truncated HDR pixels")
                row = struct.iter_unpack("4B", data)
            for r, g, b, exponent in row:
                scale = math.ldexp(1.0, exponent - 136) if exponent else 0.0
                rgb.extend((r * scale, g * scale, b * scale))
        require(not stream.read(1), "HDR has trailing bytes")
    return width, height, rgb


def paeth(a, b, c):
    p = a + b - c
    distances = (abs(p - a), abs(p - b), abs(p - c))
    return (a, b, c)[distances.index(min(distances))]


def png_rgb(path):
    require(path.stat().st_size <= 64 * 1024 * 1024, "PNG exceeds inspector's 64 MiB file limit")
    data = path.read_bytes()
    require(data[:8] == b"\x89PNG\r\n\x1a\n", "Invalid PNG signature")
    offset, header, compressed, ended = 8, None, bytearray(), False
    while offset < len(data):
        require(offset + 12 <= len(data), "Truncated PNG chunk")
        size, = struct.unpack_from(">I", data, offset)
        kind = data[offset + 4:offset + 8]
        require(offset + 12 + size <= len(data), "Truncated PNG payload")
        payload = data[offset + 8:offset + 8 + size]
        crc, = struct.unpack_from(">I", data, offset + 8 + size)
        require(zlib.crc32(kind + payload) == crc, "PNG CRC mismatch")
        offset += 12 + size
        if kind == b"IHDR":
            require(header is None and size == 13 and offset == 33, "Invalid PNG IHDR")
            header = struct.unpack(">2I5B", payload)
        elif kind == b"IDAT":
            require(header is not None, "PNG data before header")
            compressed.extend(payload)
        elif kind == b"IEND":
            require(size == 0 and offset == len(data), "Invalid PNG ending")
            ended = True
            break
        elif kind in (b"sRGB", b"gAMA"):
            require(kind == b"gAMA" and payload == struct.pack(">I", 100000), "PNG declares nonlinear encoding")
        elif kind[:1].isupper():
            raise InvalidOutput(f"Unsupported critical PNG chunk: {kind!r}")
    require(header is not None and ended, "Incomplete PNG")
    width, height, depth, color, compression, filtering, interlace = header
    require(width > 0 and height > 0 and depth == 8 and color in (2, 6) and
            compression == filtering == interlace == 0, "Unsupported PNG encoding")
    channels = 3 if color == 2 else 4
    stride = width * channels
    expected = height * (stride + 1)
    require(expected <= 64 * 1024 * 1024, "PNG exceeds inspector's 64 MiB decoded limit")
    decoder = zlib.decompressobj()
    raw = decoder.decompress(compressed, expected + 1)
    require(len(raw) == expected and decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail,
            "Incorrect/truncated PNG decoded size")
    previous, rgb = bytearray(stride), bytearray()
    for y in range(height):
        start = y * (stride + 1)
        mode, row = raw[start], bytearray(raw[start + 1:start + 1 + stride])
        require(mode <= 4, "Invalid PNG row filter")
        for x in range(stride):
            a = row[x - channels] if x >= channels else 0
            b = previous[x]
            c = previous[x - channels] if x >= channels else 0
            predictor = (0, a, b, (a + b) // 2, paeth(a, b, c))[mode]
            row[x] = (row[x] + predictor) & 255
        for x in range(0, stride, channels):
            rgb.extend(row[x:x + 3])
        previous = row
    return width, height, rgb


def read_map(path, metadata):
    fmt = metadata["cpuFormat"]
    if fmt == "RGB32F":
        require(path.suffix == ".hdr", "Float map requires Radiance HDR")
        width, height, rgb = hdr_rgb(path)
    else:
        require(fmt == "RGB8" and path.suffix == ".png", "Inspector supports PNG/RGB32F; use an independent decoder for JPEG/BMP/TGA")
        width, height, rgb = png_rgb(path)
    require((width, height) == (metadata["width"], metadata["height"]), "Map/metadata extent mismatch")
    return width, height, rgb


def inspect_model(path, metadata, map_rgb):
    # bg2-io writes four version bytes, then BIG-endian block tokens/integers,
    # irrespective of header.endianess. Only the material JSON section is read.
    with path.open("rb") as stream:
        prefix = stream.read(20)
        require(len(prefix) == 20 and prefix[:4] == b"\x00\x01\x02\x00" and
                prefix[4:8] == b"hedr" and prefix[12:16] == b"mtrl", "Unsupported BG2 material header")
        count, = struct.unpack_from(">I", prefix, 8)
        size, = struct.unpack_from(">I", prefix, 16)
        require(0 < size <= 16 * 1024 * 1024, "Invalid BG2 material JSON length")
        encoded = stream.read(size)
        require(len(encoded) == size, "Truncated BG2 materials")
        materials = parse_json(encoded)
    require(isinstance(materials, list) and count == len(materials) and count > 0, "Model/material count mismatch")
    checked = set()
    slots = ("albedoTexture", "metalnessTexture", "roughnessTexture", "normalTexture",
             "ambientOcclussion", "lightEmissionTexture")
    for material in materials:
        require(material["indirectTextureMode"] == metadata["indirectTextureMode"] and
                material["ambientOcclussionUV"] == 1 and material["ambientOcclussionScale"] == [1, 1] and
                material["ambientOcclussionChannel"] == 0 and
                material["ambientOcclussionSettings"]["color"] == 2, "Model lost baked linear UV2 semantics")
        for slot in slots:
            if not isinstance(material.get(slot), str):
                continue
            relative = material[slot]
            require(len(PurePosixPath(relative).parts) == 1, "Model texture is not local")
            resource = artifact(path.parent, relative)
            checked.add(resource)
            if slot == "ambientOcclussion":
                _, _, actual = read_map(resource, metadata)
                require(actual == map_rgb, "Model baked map pixels differ from exported image")
    return len(checked)


def inspect(manifest, expectations=None):
    manifest = Path(manifest)
    require(manifest.is_file() and not manifest.is_symlink(), "Missing/symlink manifest")
    document = parse_json(manifest.read_text(encoding="utf-8"))
    require(document["version"] == 1 and isinstance(document["targets"], list) and document["targets"], "Invalid/empty manifest")
    root, seen, reserved, generation, reports = manifest.parent, set(), set(), None, []
    expected = {} if expectations is None else expectations["targets"]
    for target in document["targets"]:
        identity = target["identity"]
        require(isinstance(identity, str) and identity and identity not in seen, "Duplicate/empty target identity")
        seen.add(identity)
        paths = {}
        for field in ("image", "metadata", "model"):
            if field not in target:
                require(field == "model", f"Missing {field} path")
                continue
            relative = target[field]
            file = artifact(root, relative)
            parts = PurePosixPath(relative).parts
            require(len(parts) >= 2 and parts[0].startswith("lightmaps-") and file not in reserved, "Invalid/colliding generation path")
            generation = generation or parts[0]
            require(generation == parts[0], "Manifest mixes generations")
            reserved.add(file)
            paths[field] = file
        metadata = parse_json(paths["metadata"].read_text(encoding="utf-8"))
        require(metadata["version"] == 1 and metadata["identity"] == identity and
                metadata["image"] == paths["image"].name and paths["metadata"].parent == paths["image"].parent,
                "Target/metadata identity mismatch")
        require(metadata["colorSpace"] == "linear" and metadata["rowZero"] == "v=0" and metadata["uvSet"] == 1 and
                metadata["signal"] == "indirectMultiplierRGB" and metadata["padding"] == "nearest-chart" and
                metadata["indirectTextureMode"] in ("bakedAO", "bakedGI"), "Invalid signal/UV/color metadata")
        require(1 <= metadata["width"] == metadata["height"] <= 16384 and
                0 <= metadata["dilationPixels"] <= metadata["width"] and
                1 <= metadata["accumulationFrames"] <= 1048576 and 1 <= metadata["samplesPerPixel"] <= 64,
                "Invalid bake settings metadata")
        floating = metadata["cpuFormat"] == "RGB32F"
        exposed = not floating and metadata["indirectTextureMode"] == "bakedGI"
        conversion = "Radiance-RGBE-quantization" if floating else "linear-exp2-exposure-clamp-round-unorm8" if exposed else "linear-clamp-round-unorm8"
        require(metadata["storageFormat"] == ("Radiance-RGBE" if floating else "RGB8") and
                metadata["exposureApplied"] == exposed and metadata["conversion"] == conversion and
                metadata["byteEncodingLossy"] is False, "Invalid encoding/conversion metadata")
        width, height, rgb = read_map(paths["image"], metadata)
        sidecars = inspect_model(paths["model"], metadata, rgb) if "model" in paths else 0
        if expectations is not None:
            require(identity in expected, f"Unexpected target: {identity}")
            wanted = expected[identity]
            for field, value in wanted.get("metadata", {}).items():
                require(metadata.get(field) == value, f"Expected {identity}/{field}={value}")
            if "model" in wanted:
                require(("model" in paths) == wanted["model"], "Model copy expectation differs")
            tolerance = wanted.get("tolerance", 0)
            require(isinstance(tolerance, (int, float)) and math.isfinite(tolerance) and tolerance >= 0, "Invalid comparison tolerance")
            for probe in wanted.get("pixels", []):
                x, y = probe["x"], probe["y"]
                require(type(x) is int and type(y) is int and 0 <= x < width and 0 <= y < height and len(probe["rgb"]) == 3,
                        "Invalid pixel probe")
                actual = rgb[(y * width + x) * 3:(y * width + x) * 3 + 3]
                require(all(math.isfinite(v) and abs(a - v) <= tolerance for a, v in zip(actual, probe["rgb"])),
                        f"Pixel/orientation mismatch: {identity} ({x},{y})")
            if "maxAbove" in wanted:
                require(max(rgb) > wanted["maxAbove"], "Expected HDR range was not preserved")
        reports.append({"identity": identity, "width": width, "height": height, "format": metadata["cpuFormat"],
                        "minimum": min(rgb), "maximum": max(rgb), "modelSidecarsChecked": sidecars,
                        "imageSHA256": hashlib.sha256(paths["image"].read_bytes()).hexdigest()})
    if expectations is not None:
        require(seen == set(expected), "Missing expected targets")
    return {"generation": generation, "targets": reports,
            "scope": "artifact pixels/metadata/material sidecars; not engine reload, geometry, lighting correctness or native execution evidence"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("--expect", type=Path, help="Independent per-identity metadata/pixel expectations")
    args = parser.parse_args()
    try:
        expectations = parse_json(args.expect.read_text(encoding="utf-8")) if args.expect else None
        print(json.dumps(inspect(args.manifest, expectations), indent=2, allow_nan=False))
    except (ValueError, KeyError, TypeError, OSError, struct.error, zlib.error) as error:
        parser.exit(1, f"Invalid output: {error}\n")


if __name__ == "__main__":
    main()
