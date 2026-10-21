"""Static M09-04 source checks. No build, engine or GPU work is executed."""
from pathlib import Path
import csv
import hashlib
import re

ROOT = Path(__file__).resolve().parents[4]
OUT = Path(__file__).resolve().parent
CODE = {'.hpp', '.h', '.cpp', '.c', '.mm', '.m'}
INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^>"\n]+)[>"]', re.M)
LEGACY = re.compile(r'bg2e/render/|\brender::|scene/vk/|\bscene::vk\b')
NATIVE_CALL = re.compile(r'\b(?:vk[A-Z]\w*|vma[A-Z]\w*|gl[A-Z]\w*|SDL_(?:Vulkan|Metal|GL)_\w*)\s*\(')
NATIVE_HEADER = re.compile(r'(?:vulkan/|vk_mem_alloc|SDL_(?:vulkan|metal|opengl)|imgui_impl_(?:vulkan|metal|opengl))')
HIGHER_INCLUDE = re.compile(r'bg2e/(?!gpu/|base/|app/|geo/|math/|common\.hpp)')


def code_only(text):
    return re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', '', text, flags=re.S)


def main():
    sources = [p for tree in ('lib/include/bg2e', 'lib/src/bg2e', 'apps/draw', 'examples/draw', 'examples/gpu', 'tests')
               for p in (ROOT / tree).rglob('*') if p.is_file() and p.suffix in CODE]
    for tree in ('lib/include/bg2e/render', 'lib/src/bg2e/render'):
        assert not any(p.is_file() for p in (ROOT / tree).rglob('*')), tree
    for path in sources:
        text = path.read_text()
        assert not LEGACY.search(text), path
        for token in INCLUDE.findall(text):
            if token.startswith('bg2e/'):
                assert (ROOT / 'lib/include' / token).is_file(), (path, token)
        if path.is_relative_to(ROOT / 'lib'):
            native = '/gpu/' in path.as_posix()
            if not native:
                assert not NATIVE_CALL.search(code_only(text)), path
                assert not any(NATIVE_HEADER.search(t) for t in INCLUDE.findall(text)), path
            else:
                assert not any(HIGHER_INCLUDE.search(t) for t in INCLUDE.findall(text)), path
        if path.is_relative_to(ROOT / 'lib/include/bg2e'):
            assert not any('imgui' in t.lower() for t in INCLUDE.findall(text)), path
    vma_owners = [p for p in sources if re.search(r'^\s*#\s*define\s+VMA_IMPLEMENTATION\b', p.read_text(), re.M)]
    assert len(vma_owners) == 1 and vma_owners[0].is_relative_to(ROOT / 'lib/src/bg2e/gpu/vk'), vma_owners
    owner_text = vma_owners[0].read_text()
    definition = owner_text.index('#define VMA_IMPLEMENTATION')
    assert owner_text.find('#include <vma/vk_mem_alloc.h>', definition) > definition
    print('VMA implementation owner:', vma_owners[0].relative_to(ROOT))
    print('Supported source legacy/include/native/dependency/header checks:', len(sources))
    rows = list(csv.DictReader((OUT / 'native-changes.tsv').open(), delimiter='\t'))
    correction_log = OUT / 'validation-corrections.tsv'
    corrections = list(csv.DictReader(correction_log.open(), delimiter='\t')) if correction_log.exists() else []
    for row in rows:
        path = ROOT / row['path']
        if row['action'] == 'removed':
            assert not path.exists(), path
        else:
            expected = row['sha256_after']
            for correction in corrections:
                if correction['path'] == row['path']:
                    assert correction['sha256_before'] == expected, path
                    expected = correction['sha256_after']
            assert hashlib.sha256(path.read_bytes()).hexdigest() == expected, path
            assert path.read_text().startswith('/*\n *    business grade graphic engine'), path
            assert 'GNU General Public License' in path.read_text()[:1000], path
    removed_noise = next(r for r in rows if r['path'] == 'lib/src/bg2e/render/blue_noise_data.h')
    noise = ROOT / 'lib/src/bg2e/utils/blue_noise_data.h'
    assert hashlib.sha256(noise.read_bytes()).hexdigest() == removed_noise['sha256_before']
    assert '#define BG2E_BLUE_NOISE_WIDTH 128' in noise.read_text()
    assert '#define BG2E_BLUE_NOISE_LAYERS 4' in noise.read_text()
    print('Source delta hashes/GPL and exact noise relocation:', len(rows))


if __name__ == '__main__':
    main()
