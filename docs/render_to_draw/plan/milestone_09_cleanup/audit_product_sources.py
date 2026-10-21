"""Nonexecuting M09-05 source/deployment audit; does not invoke build tools."""
from pathlib import Path
import collections
import csv
import hashlib
import re

ROOT = Path(__file__).resolve().parents[4]
OUT = Path(__file__).resolve().parent


def main():
    rows = list(csv.DictReader((OUT / 'product-changes.tsv').open(), delimiter='\t'))
    for row in rows:
        path = ROOT / row['path']
        if row['action'] == 'removed':
            assert not path.exists(), path
        else:
            assert hashlib.sha256(path.read_bytes()).hexdigest() == row['sha256_after'], path
    print('Product delta:', dict(collections.Counter(r['action'] for r in rows)))
    assert {p.name for p in (ROOT / 'examples').iterdir() if p.is_dir()} == {'draw', 'gpu'}
    assert not (ROOT / 'shaders/src/glsl').exists()
    retired = re.compile(r'bg2e/render/|\brender::|scene/vk/|\bscene::vk\b|src/bg2e/render|shaders/src/glsl')
    count = 0
    for tree in ('lib', 'apps', 'examples', 'tests', 'scripts', 'cmake', 'shaders'):
        for path in (ROOT / tree).rglob('*'):
            if not path.is_file() or 'third_party' in path.parts:
                continue
            if path.suffix not in {'.cpp', '.hpp', '.h', '.c', '.glsl', '.metal', '.cmake', '.in', '.sh', '.ps1', '.bat', '.txt'}:
                continue
            assert not retired.search(path.read_text()), path
            count += 1
    registrations = 0
    for path in ROOT.rglob('CMakeLists.txt'):
        if not any(path.is_relative_to(ROOT / t) for t in ('apps', 'examples', 'tests')):
            continue
        for subdirectory in re.findall(r'^\s*add_subdirectory\(([^\s)$]+)\)', path.read_text(), re.M):
            assert (path.parent / subdirectory / 'CMakeLists.txt').is_file(), (path, subdirectory)
            registrations += 1
    edges, stages = 0, 0
    outputs = set()
    for name in ('draw', 'bake'):
        tree = ROOT / 'shaders/src' / name
        for path in tree.rglob('*'):
            if path.suffix not in {'.glsl', '.metal'}:
                continue
            for token in re.findall(r'^\s*#\s*include\s*"([^"\n]+)"', path.read_text(), re.M):
                assert (path.parent / token).is_file() or (tree / token).is_file(), (path, token)
                edges += 1
            if re.search(r'\.(vert|frag|comp|rgen|rmiss|rchit|rahit|rint)\.(glsl|metal)$', path.name):
                stages += 1
                relative = str(path.relative_to(ROOT / 'shaders/src'))
                output = re.sub(r'\.(glsl|metal)$', '.spv' if path.suffix == '.glsl' else '.metallib', relative)
                assert output not in outputs, output
                outputs.add(output)
    cmake = (ROOT / 'lib/CMakeLists.txt').read_text()
    assert cmake.count('compile_paired_shader_tree(') == 2
    assert 'compile_shaders(' not in cmake and 'COMPONENT RuntimeShaders' in cmake
    for tree in ('draw', 'bake'):
        assert '${DST_SHADER_DIR}/' + tree in cmake
    for name in ('create_standalone_project.sh', 'create_standalone_project.ps1', 'update_libraries.sh', 'update_libraries.ps1'):
        text = (ROOT / 'scripts' / name).read_text()
        assert 'gbuffer.frag.spv' in text and 'uv_surface.frag.spv' in text, name
        assert 'draw' in text and 'bake' in text, name
    print('Legacy-free runtime/build/script sources:', count)
    print('Literal application/example/test registrations:', registrations)
    print('Paired shader stage outputs (no collisions):', stages, 'include edges:', edges)
    print('Source checks only; no configure, compilation, script execution or native result.')


if __name__ == '__main__':
    main()
