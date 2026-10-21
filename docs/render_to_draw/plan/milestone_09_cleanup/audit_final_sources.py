"""M09-06 lexical API/umbrella/product audit. Never configures or runs engine code.

Outputs source evidence only, including a literal standalone-template projection.
This is not preprocessing, compilation, exported-binary inspection or a creator run.
"""
from pathlib import Path
import argparse
import csv
import hashlib
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[4]
HERE = Path(__file__).resolve().parent
INCLUDES = re.compile(r'^\s*#\s*include\s*[<"]([^>"\n]+)[>"]', re.M)
RETIRED = re.compile(
    r'bg2e/render/|\brender\s*::|scene/vk/|\bscene\s*::\s*vk\b|'
    r'\bnamespace\s+(?:bg2e\s*::\s*)?render\b|'
    r'\bscene::(?:Drawable(?:Base)?|Mesh|RenderMesh|DrawVisitor|RenderQueueVisitor)\b|'
    r'\b(?:ProductionRender|ExperimentalDraw|RenderGraphicsExecution)\b')
NATIVE = re.compile(r'^(?:vulkan/|Metal/|Foundation/|QuartzCore/)|imgui|vk_mem_alloc|SDL_(?:vulkan|metal|opengl)')
CODE = {'.cpp', '.hpp', '.h', '.c', '.mm', '.m', '.glsl', '.metal', '.inc'}
PUBLIC = ROOT / 'lib/include'
ROOT_HEADERS = ['bg2e.hpp', 'bg2e/all.hpp'] + [f'bg2e/{n}/all.hpp' for n in ('app', 'scene', 'db', 'ui', 'draw', 'bake', 'manipulation', 'utils', 'gpu')]


def digest(paths):
    lines = [f'{p.relative_to(ROOT).as_posix()}\t{hashlib.sha256(p.read_bytes()).hexdigest()}\n' for p in sorted(paths)]
    return hashlib.sha256(''.join(lines).encode()).hexdigest()


def tsv(output, name, fields, rows):
    with (output / name).open('w', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n')
        writer.writerow(fields)
        writer.writerows(rows)


def closure(root):
    visited, external, pending = set(), set(), [root]
    while pending:
        path = pending.pop()
        if path in visited:
            continue
        visited.add(path)
        text = path.read_text()
        assert not RETIRED.search(text), path
        for token in INCLUDES.findall(text):
            candidate = PUBLIC / token
            local = path.parent / token
            if candidate.is_file():
                pending.append(candidate)
            elif local.is_file() and local.is_relative_to(PUBLIC):
                pending.append(local)
            else:
                assert not token.startswith('bg2e/'), (path, token)
                external.add(token)
    return visited, external


def main(output):
    output.mkdir(parents=True, exist_ok=True)
    closures = []
    reachable = set()
    for name in ROOT_HEADERS:
        files, external = closure(PUBLIC / name)
        assert not any(NATIVE.search(t) for t in external), (name, external)
        closures.append((name, len(files), digest(files), ';'.join(sorted(external))))
        reachable.update(files)
    tsv(output, 'umbrella-closure.tsv', ('root', 'lexical_header_count', 'closure_sha256', 'external_include_tokens'), closures)
    headers = sorted(p for p in PUBLIC.rglob('*') if p.is_file() and p.suffix in {'.hpp', '.h'})
    api = []
    for path in headers:
        text = path.read_text()
        assert not RETIRED.search(text), path
        assert not any('imgui' in t.lower() for t in INCLUDES.findall(text)), path
        clean = re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)
        names = re.findall(r'\b(?:class|struct)\s+(?:BG2E_API\s+)?(\w+)|\benum\s+(?:class\s+)?(\w+)|^\s*using\s+(\w+)\s*=', clean, re.M)
        identifiers = sorted({name for row in names for name in row if name})
        exports = [' '.join(line.split()) for line in clean.splitlines() if 'BG2E_API' in line]
        api.append((path.relative_to(ROOT).as_posix(), hashlib.sha256(path.read_bytes()).hexdigest(),
                    'yes' if path in reachable else 'no', ';'.join(identifiers), ' | '.join(exports)))
    tsv(output, 'public-api-evidence.tsv', ('header', 'sha256', 'umbrella_reachable', 'lexical_type_alias_names', 'export_marker_lines'), api)
    products = []
    for group in ('apps', 'examples', 'tests'):
        registration = ROOT / group / 'CMakeLists.txt'
        for directory in re.findall(r'^\s*add_subdirectory\(([^\s)$]+)\)', registration.read_text(), re.M):
            path = ROOT / group / directory
            cmake = path / 'CMakeLists.txt'
            match = re.search(r'^\s*set\(APP_TARGET_NAME\s+(\w+)\)', cmake.read_text(), re.M)
            assert match, cmake
            source = sorted((path / 'src').glob('*.cpp'))
            assert source, path
            products.append((group, match.group(1), path.relative_to(ROOT).as_posix(),
                             ';'.join(p.relative_to(ROOT).as_posix() for p in source), digest(source)))
    assert len(products) == 33 and len({p[1] for p in products}) == 33
    assert {p[1] for p in products if p[0] == 'apps'} == {'draw_model_edit', 'draw_bg2e_composer', 'draw_lightmap_generator'}
    tsv(output, 'final-products.tsv', ('group', 'target', 'directory', 'cpp_sources', 'cpp_source_sha256'), products)
    # Literal substitutions performed by both creator scripts, without running
    # either creator, copying binaries/assets, or invoking CMake.
    project = 'm09_validation_app'
    projected = {
        'CMakeLists.txt': (ROOT / 'scripts/templates/CMakeLists.txt.in').read_text().replace('${PROJECT_NAME}', project),
        'app/CMakeLists.txt': (ROOT / 'scripts/templates/app_CMakeLists.txt.in').read_text().replace('${PROJECT_NAME}', project),
        'app/src/main.cpp': (ROOT / 'scripts/templates/main.cpp').read_text(),
    }
    assert f'project({project})' in projected['CMakeLists.txt']
    assert f'set(APP_TARGET_NAME {project})' in projected['app/CMakeLists.txt']
    for name, text in projected.items():
        assert not RETIRED.search(text) and '${PROJECT_NAME}' not in text, name
    template_closure, external = set(), set()
    for token in INCLUDES.findall(projected['app/src/main.cpp']):
        if (PUBLIC / token).is_file():
            files, includes = closure(PUBLIC / token)
            template_closure.update(files)
            external.update(includes)
    assert not any(NATIVE.search(t) for t in external)
    source_paths = sorted({p for tree in ('lib/include', 'lib/src', 'apps', 'examples', 'tests', 'scripts', 'cmake', 'shaders', 'docs/profiling_and_stats')
                           for p in (ROOT / tree).rglob('*') if p.is_file() and 'third_party' not in p.parts and
                           (p.suffix in CODE | {'.cmake', '.in', '.sh', '.ps1', '.bat'} or p.name == 'CMakeLists.txt')} |
                          {ROOT / 'CMakeLists.txt', ROOT / 'lib/CMakeLists.txt'})
    for path in source_paths:
        assert not RETIRED.search(path.read_text()), path
    # Confirm chronological delta chains agree, rather than checking an obsolete
    # intermediate hash against a legitimately changed final file.
    final = {}
    for name in ('entrypoint-changes.tsv', 'shared-api-changes.tsv', 'native-changes.tsv', 'product-changes.tsv', 'validation-corrections.tsv'):
        if not (HERE / name).exists():
            continue
        for row in csv.DictReader((HERE / name).open(), delimiter='\t'):
            if row['path'] in final:
                previous = '' if row['sha256_before'] == 'absent' else row['sha256_before']
                assert final[row['path']] == previous, (name, row['path'])
            final[row['path']] = '' if row['sha256_after'] == 'absent' else row['sha256_after']
    for name, expected in final.items():
        path = ROOT / name
        if expected:
            assert path.is_file() and hashlib.sha256(path.read_bytes()).hexdigest() == expected, name
        else:
            assert not path.exists(), name
    report = {
        'evidence_kind': 'nonexecuting_lexical_source_audit',
        'head': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
        'revision_scope': 'HEAD plus inherited uncommitted source; hashes identify audited files',
        'source_files': len(source_paths), 'source_set_sha256': digest(source_paths),
        'public_headers': len(headers), 'umbrella_roots': len(closures), 'umbrella_union_headers': len(reachable),
        'registered_products': len(products), 'final_delta_paths': len(final),
        'removed_delta_paths': sum(not value for value in final.values()),
        'standalone_projection': {name: hashlib.sha256(text.encode()).hexdigest() for name, text in projected.items()},
        'standalone_header_count': len(template_closure), 'standalone_closure_sha256': digest(template_closure),
        'missing_bg2e_includes': 0, 'retired_source_matches': 0,
        'execution': {'configure': False, 'compile': False, 'native_test': False, 'artifact_inspection': False,
                      'standalone_creator': False, 'binary_symbol_inspection': False},
        'limits': 'All conditional includes traversed lexically; external SDK/system headers not expanded. Type/export names are lexical declarations, not binary symbols. Projection is not creator execution or configured standalone build.',
    }
    (output / 'final-source-audit.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, default=HERE)
    main(parser.parse_args().output_dir)
