#!/usr/bin/env python3
"""Reproducible spatial diagnostics for the generated RG8 header."""
import math
import re
import sys

SIZE = 128
COUNT = SIZE * SIZE


def analyze(path):
    source = open(path, encoding="ascii").read()
    layers = int(re.search(r"#define BG2E_BLUE_NOISE_LAYERS (\d+)", source).group(1))
    data = list(map(int, re.findall(r"\b\d+\b", source.split("= {", 1)[1].split("};", 1)[0])))
    assert len(data) == COUNT * 2 * layers
    roots = [
        [complex(math.cos(-2 * math.pi * k * i / SIZE),
                 math.sin(-2 * math.pi * k * i / SIZE)) for i in range(SIZE)]
        for k in range(1, 5)
    ]
    powers = []
    correlations = []
    pair_chi2 = []
    for layer in range(layers):
        pair_bins = [0] * 256
        channels = [[data[(layer * COUNT + i) * 2 + c] for i in range(COUNT)] for c in range(2)]
        for a, b in zip(*channels):
            pair_bins[(a // 16) * 16 + b // 16] += 1
        expected = COUNT / 256
        pair_chi2.append(sum((n - expected) ** 2 / expected for n in pair_bins) / 255)
        mean = [sum(ch) / COUNT for ch in channels]
        covariance = sum((a - mean[0]) * (b - mean[1]) for a, b in zip(*channels)) / COUNT
        variance = [sum((v - m) ** 2 for v in ch) / COUNT for ch, m in zip(channels, mean)]
        correlations.append(covariance / math.sqrt(variance[0] * variance[1]))
        for ch in channels:
            # A threshold mask is the relevant distribution for stochastic decisions.
            mask = [1 if v < 128 else 0 for v in ch]
            row_dft = [[sum(mask[y * SIZE + x] * roots[k][x] for x in range(SIZE))
                        for y in range(SIZE)] for k in range(4)]
            energy = []
            for ky in range(4):
                for kx in range(4):
                    energy.append(abs(sum(row_dft[kx][y] * roots[ky][y]
                                          for y in range(SIZE))) ** 2)
            powers.append(sum(energy) / len(energy) / (COUNT * 0.25))
    print(f"{path}: layers={layers}, threshold low-frequency power={sum(powers)/len(powers):.4f}, "
          f"channel correlation={sum(correlations)/layers:.4f}, "
          f"16x16 pair chi2/dof={sum(pair_chi2)/layers:.4f}")


if __name__ == "__main__":
    for name in sys.argv[1:]:
        analyze(name)
