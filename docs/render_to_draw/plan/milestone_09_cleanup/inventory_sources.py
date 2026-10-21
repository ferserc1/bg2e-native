"""Generate the M09 retirement manifest and textual dependency evidence.

Run from any directory. Reads tracked and untracked nonignored project sources.
Writes adjacent TSVs by default; use --output-dir to preserve historical snapshots.
No engine, build, shader compiler, application or test is executed. Includes are
resolved lexically, not preprocessed. Evidence includes comments/strings and is
not a semantic call graph. Curated decisions live in retirement-manifest.md.
"""
from pathlib import Path
import argparse
import collections
import csv
import hashlib
import re
import subprocess

ROOT = Path(__file__).resolve().parents[4]
OUT = Path(__file__).resolve().parent
OUTPUT = OUT
LEGACY = re.compile(
    r'bg2e/render/|\brender::|\bnamespace\s+bg2e::render\b|scene/vk/|\bscene::vk\b|'
    r'\bscene::(?:Drawable(?:Base)?|Mesh|RenderMesh|DrawVisitor|RenderQueueVisitor)\b|'
    r'\b(?:ProductionRender|ExperimentalDraw|RenderGraphicsExecution|createRenderGraphicsExecution)\b|'
    r'(?:\.\./)?render/blue_noise_data\.h\b')
NATIVE = re.compile(r'\b(?:Vk[A-Z]\w*|vk[A-Z]\w*|Vma[A-Z]\w*|SDL_(?:Vulkan|Metal|GL)_\w*|gl[A-Z]\w*)\b|\bMTL::|imgui_impl_(?:vulkan|metal|opengl)')
REGISTRATION = re.compile(r'add_subdirectory|bundle_app|RuntimeShaders|paired_shaders|shaders/src/|scripts/templates|APP_TARGET_NAME')
INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.M)
TREES = ('lib/include/', 'lib/src/', 'apps/', 'examples/', 'tests/', 'cmake/', 'scripts/', 'shaders/')
SUFFIXES = {'.cpp', '.hpp', '.c', '.h', '.glsl', '.metal', '.cmake', '.sh', '.ps1', '.json', '.txt', '.in', '.py', '.mm', '.m', '.inc', '.bat'}
RENDER = ('lib/include/bg2e/render/', 'lib/src/bg2e/render/')
VK_SCENE = ('lib/include/bg2e/scene/vk/', 'lib/src/bg2e/scene/vk/')
OLD_APPS = ('apps/model_edit/', 'apps/bg2e_composer/', 'apps/lightmap_generator/')
KEEP = ('lib/include/bg2e/gpu/', 'lib/src/bg2e/gpu/', 'lib/include/bg2e/draw/', 'lib/src/bg2e/draw/', 'lib/include/bg2e/bake/', 'lib/src/bg2e/bake/', 'shaders/src/draw/', 'shaders/src/bake/')
SCENE_REMOVE = {'Drawable.hpp', 'Drawable.cpp', 'Mesh.hpp', 'DrawVisitor.hpp', 'DrawVisitor.cpp', 'RenderQueueVisitor.hpp', 'RenderQueueVisitor.cpp', 'SkyDomeTextureGenerator.hpp', 'SkyDomeTextureGenerator.cpp'}


def classify(name, text):
    path = Path(name)
    if name == 'lib/src/bg2e/utils/blue_noise_data.h':
        return 'keep', 'M09-04', 'authoritative CPU blue-noise table relocated byte-identically from retired render tree'
    if name == 'lib/src/bg2e/render/blue_noise_data.h':
        return 'relocate_data', 'M09-04', 'utils/blue_noise_data.h; preserve exact bytes and update utils private include before render removal'
    if name == 'lib/src/bg2e/app/detail/RenderGraphicsExecution.cpp':
        return 'remove', 'M09-02', 'DrawGraphicsExecution becomes ordinary windowed execution; preserve stopped updates and failure teardown'
    if name.startswith(VK_SCENE):
        return 'remove', 'M09-03', 'draw DataBinding/ShaderData and gpu resource sets replace production scene bindings'
    if name.startswith(RENDER):
        return 'remove', 'M09-04', 'draw Engine/RendererDeferred/passes, bake services and gpu backend; no forwarding API; migrate incoming users first'
    if name.startswith(OLD_APPS):
        return 'remove', 'M09-02/05', 'corresponding apps/draw product; remove old target and source after canonical entrypoint migration'
    if name.startswith('examples/') and not name.startswith(('examples/draw/', 'examples/gpu/')) and name != 'examples/CMakeLists.txt':
        return 'remove', 'M09-05', 'draw/gpu examples retain setup/compute/geometry/UI/environment/scene/material/headless demonstrations; no legacy forward/MSAA product'
    if name.startswith(('lib/include/bg2e/scene/', 'lib/src/bg2e/scene/')) and path.name in SCENE_REMOVE:
        return 'remove', 'M09-03', 'draw Drawable/Mesh/RenderQueueVisitor/environment generators replace native scene types; keep neutral scene visitors'
    if path.name in ('LightmapOutputWriter.hpp', 'LightmapOutputWriter.cpp') and '/db/' in name:
        return 'remove', 'M09-03', 'bake LightmapOutputWriter and generic DB file/image/archive IO; retire old flat output writer'
    if name.startswith('tests/test_01_'):
        return 'migrate', 'M09-05', 'port key/mouse input assertions to draw delegate; retain event behavior'
    if name.startswith(('tests/test_02_', 'tests/test_03_')):
        return 'migrate', 'M09-05', 'port UV surface/RGB material/exact sample/reset checks to draw/bake; preserve assertions before retiring native fixture'
    if name.startswith('tests/test_04_'):
        return 'keep', 'M09-05', 'neutral CPU UV2 atlas regression and its target survive'
    if name.startswith('shaders/src/glsl/'):
        return 'remove', 'M09-05', 'legacy-only stages after UI and product migration; retain draw/bake paired shaders and vendored SMAA tables'
    if name.startswith(('cmake/', 'scripts/')) or path.name == 'CMakeLists.txt' or name in ('shaders/build.sh', 'shaders/build.bat'):
        return 'migrate', 'M09-05', 'keep platform SDK/install/runtime/package behavior; migrate obsolete registrations/template/default paths only'
    if name.startswith(KEEP):
        return ('migrate' if LEGACY.search(text) else 'keep'), 'M09-03/04/05', 'retain optimized gpu/draw/bake and paired shader contracts; remove only coexistence references'
    if name.startswith(('apps/draw/', 'examples/draw/', 'examples/gpu/', 'tests/')):
        return 'migrate', 'M09-02/05', 'retain supported product/probe; update canonical app/accessor/delegate names and transitive includes'
    if LEGACY.search(text):
        return 'migrate', 'M09-02/03/04', 'retain neutral/shared functions and draw branch; remove production overloads/types/exports; see family decisions'
    if NATIVE.search(text):
        return 'migrate', 'M09-04', 'move actual native graphics implementation behind gpu bridges; retain CPU wrappers and legitimate native-handle seams'
    return 'keep', 'M09-03/06', 'neutral engine/CPU/IO functionality; preserve data and material revisions; check transitive includes'


def write_tsv(name, fields, rows):
    with (OUTPUT / name).open('w', newline='') as stream:
        writer = csv.writer(stream, delimiter='\t', lineterminator='\n')
        writer.writerow(fields)
        writer.writerows(rows)


def main(output_dir=None):
    global OUTPUT
    OUTPUT = Path(output_dir) if output_dir else OUT
    OUTPUT.mkdir(parents=True, exist_ok=True)
    tracked = subprocess.check_output(['git', 'ls-files', '--cached', '--others', '--exclude-standard', '-z'], cwd=ROOT).decode().split('\0')
    names = sorted(n for n in tracked if n and (ROOT / n).is_file() and
                   (n == 'CMakeLists.txt' or n.startswith(TREES)) and Path(n).suffix in SUFFIXES)
    sources = {n: (ROOT / n).read_text(errors='replace') for n in names}
    edges, evidence = [], []
    incoming = collections.defaultdict(set)
    for name, source in sources.items():
        for match in INCLUDE.finditer(source):
            token = match.group(1)
            shader_root = next((ROOT / tree for tree in ('shaders/src/draw', 'shaders/src/bake', 'shaders/src/glsl') if name.startswith(tree + '/')), ROOT / 'shaders/src')
            choices = (ROOT / Path(name).parent / token, ROOT / 'lib/include' / token, shader_root / token)
            for candidate in choices:
                resolved = candidate.resolve()
                if resolved.is_relative_to(ROOT) and resolved.relative_to(ROOT).as_posix() in sources:
                    target = resolved.relative_to(ROOT).as_posix()
                    line = source.count('\n', 0, match.start()) + 1
                    edges.append((name, line, target, token))
                    incoming[target].add(name)
                    break
        for line, value in enumerate(source.splitlines(), 1):
            for kind, pattern in (('legacy', LEGACY), ('native', NATIVE), ('registration', REGISTRATION)):
                if pattern.search(value):
                    evidence.append((name, line, kind, value.strip()))
    decisions = {name: classify(name, source) for name, source in sources.items()}
    # A neutral file may export a deleted header without spelling its types.
    # Mark those include consumers as migration work rather than silently KEEP.
    for consumer, line, dependency, token in edges:
        if decisions[consumer][0] == 'keep' and decisions[dependency][0] in ('remove', 'relocate_data'):
            decisions[consumer] = ('migrate', 'M09-03/04/05', 'retain neutral behavior; remove/replace included retired header; see include-edges.tsv')
    manifest = []
    for name, source in sources.items():
        action, owners, reason = decisions[name]
        manifest.append((name, action, owners, reason, len(incoming[name]),
                         hashlib.sha256((ROOT / name).read_bytes()).hexdigest()))
    baseline = {r['path']: r for r in csv.DictReader((OUT.parent / 'milestone_08_lightmap_generator/retirement-inventory.tsv').open(), delimiter='\t')}
    retired = {}
    for change_log in (OUT / 'entrypoint-changes.tsv', OUT / 'shared-api-changes.tsv', OUT / 'native-changes.tsv', OUT / 'product-changes.tsv'):
        if change_log.exists():
            with change_log.open() as stream:
                retired.update({r['path']: r for r in csv.DictReader(stream, delimiter='\t') if r['action'] == 'removed'})
    missing = set(baseline) - set(sources)
    assert missing <= set(retired), 'A baseline path disappeared without an explicit retirement record'
    write_tsv('retirement-manifest.tsv', ('path', 'action', 'owner_steps', 'replacement_or_preservation', 'incoming_files', 'sha256'), manifest)
    write_tsv('include-edges.tsv', ('consumer', 'line', 'dependency', 'include_token'), sorted(edges))
    write_tsv('reference-evidence.tsv', ('path', 'line', 'kind', 'text'), evidence)
    write_tsv('baseline-comparison.tsv', ('path', 'M08_disposition', 'M09_action', 'M09_owner_steps'),
              [(r[0], baseline.get(r[0], {}).get('disposition', 'newly_covered'), r[1], r[2]) for r in manifest] +
              [(name, baseline[name]['disposition'], 'removed', retired[name]['owner_step']) for name in sorted(missing)])

    print('HEAD:', subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT).decode().strip())
    print('Files:', len(manifest), 'actions:', dict(collections.Counter(r[1] for r in manifest)))
    print('Legacy files:', len({r[0] for r in evidence if r[2] == 'legacy'}), 'include edges:', len(edges), 'evidence:', len(evidence))
    print('Newly covered:', len(set(sources) - set(baseline)))
    for name in ('retirement-manifest.tsv', 'include-edges.tsv', 'reference-evidence.tsv', 'baseline-comparison.tsv'):
        print(name, hashlib.sha256((OUTPUT / name).read_bytes()).hexdigest())


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path)
    main(parser.parse_args().output_dir)
