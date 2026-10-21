"""Refresh M08 review candidate inventory from the repository root.

Read-only for engine/build files; replaces only retirement-inventory.tsv beside
this review. Dispositions are migration candidates, never deletion commands.
Includes direct textual matches plus whole relevant implementation/product trees;
comments and string matches remain visible, and this is not a transitive call graph.
"""
from pathlib import Path
import re,collections,hashlib
root=Path('.')
out=Path('docs/render_to_draw/plan/milestone_08_lightmap_generator')
patterns={
 'legacy':re.compile(r'bg2e/render/|\brender::|scene/vk/|\bscene::vk\b'),
 'native':re.compile(r'\b(?:Vk[A-Z]\w*|vk[A-Z]\w*|Vma[A-Z]\w*|SDL_Vulkan_\w*)\b|[<\"]vulkan/'),
 'build_product':re.compile(r'lightmap_generator|model_edit|bg2e_composer|ProductionRender|ExperimentalDraw|RuntimeShaders|paired_shaders|render/|scene/vk'),
}
legacy_roots=('lib/include/bg2e/render/','lib/src/bg2e/render/','lib/include/bg2e/scene/vk/','lib/src/bg2e/scene/vk/')
old_apps=('apps/model_edit/','apps/bg2e_composer/','apps/lightmap_generator/')
keep_prefix=('lib/include/bg2e/gpu/','lib/src/bg2e/gpu/','lib/include/bg2e/draw/','lib/src/bg2e/draw/','lib/include/bg2e/bake/','lib/src/bg2e/bake/','shaders/src/draw/','shaders/src/bake/')
rows=[]
for folder in ('lib/include','lib/src','apps','examples','tests','cmake','scripts','shaders'):
 for p in sorted(Path(folder).rglob('*')):
  if not p.is_file() or p.suffix not in ('.cpp','.hpp','.c','.h','.glsl','.metal','.cmake','.sh','.ps1','.json','.txt','.in','.py','.mm','.m','.inc','.bat'):continue
  if '__pycache__' in p.parts:continue
  name=p.as_posix();s=p.read_text(errors='replace')
  signals=[k for k,pattern in patterns.items() if pattern.search(s)]
  legacy_example = name.startswith('examples/') and not name.startswith(('examples/draw/','examples/gpu/'))
  forced=(legacy_example or name.startswith('tests/test_') or name in ('shaders/build.sh','shaders/build.bat') or name.startswith(legacy_roots+old_apps+keep_prefix+('apps/draw/','examples/draw/','examples/gpu/','shaders/src/glsl/')) or p.name=='CMakeLists.txt' or name.startswith('cmake/') or (name.startswith('scripts/') and p.suffix in ('.sh','.ps1')))
  if not signals and not forced:continue
  if name.startswith(legacy_roots):action,phase,reason='remove_after_gates','M09-03/04','production implementation or scene native bindings; resolve incoming users first'
  elif name in ('lib/include/bg2e/db/LightmapOutputWriter.hpp','lib/src/bg2e/db/LightmapOutputWriter.cpp'):
   action,phase,reason='retire_specialized_legacy','M09-03','remove production render::LightmapPixels writer after callers retire; keep bake writer and generic DB IO'
  elif name.startswith(old_apps):action,phase,reason='retire_product','M09-02/05','old app source/target; replacement lives in apps/draw'
  elif name.startswith('examples/') and not name.startswith(('examples/draw/','examples/gpu/')) and name!='examples/CMakeLists.txt':action,phase,reason='retire_or_port_check','M09-05','legacy example; preserve a required demonstration via delivered draw/gpu product'
  elif name.startswith('tests/test_'):action,phase,reason='inspect_port_or_keep','M09-05','registered test may contain neutral atlas/input checks; inspect full dependencies before removal'
  elif name.startswith('shaders/src/glsl/'):action,phase,reason='audit_shared_before_remove','M09-05','legacy shader tree; resolve shared includes and runtime stage references'
  elif name.startswith(('apps/draw/','examples/draw/','examples/gpu/')):action,phase,reason='keep_product_migrate_seams','M09-02/05','surviving product; resolve umbrella/transitive legacy and canonical identifiers'
  elif name.startswith(keep_prefix):action,phase,reason='keep_current_implementation','M09-03/04/05','retain current public draw/bake or gpu native/shader implementation; migrate only proven coexistence seams'
  elif p.name=='CMakeLists.txt' or name.startswith(('cmake/','scripts/')) or name in ('shaders/build.sh','shaders/build.bat'):action,phase,reason='migrate_build_or_template','M09-05','target/resource/install/template registration; keep required SDK and paired shader support'
  elif 'legacy' in signals:action,phase,reason='migrate_shared_api','M09-02/03/04','dual/shared API or native UI/app seam; retain neutral behavior and live draw implementation'
  elif 'native' in signals:action,phase,reason='audit_native_boundary','M09-04','classify real native calls versus legitimate binding indices/casts before relocating'
  else:action,phase,reason='keep_verify_consumer','M09-05/06','retained probes/settings/output consumers; reconcile final names and encoded data contracts'
  rows.append((name,action,phase,','.join(signals) or 'tree_inventory',reason))
# Root registrations are outside the searched trees.
p=Path('CMakeLists.txt');rows.append((str(p),'migrate_build_or_template','M09-05','build_product','preserve current options/platform/SDK rules; remove only obsolete registrations'))
rows=sorted(set(rows))
with (out/'retirement-inventory.tsv').open('w') as f:
 f.write('path\tdisposition\towner_steps\tsignals\treason\n')
 for row in rows:f.write('\t'.join(row)+'\n')
counts=collections.Counter(r[1] for r in rows)
print(len(rows),'candidate source/build/resource files')
print(dict(sorted(counts.items())))
# Record direct scan separately; scan line counts are not transitive dependency proof.
print('Direct legacy matches:',sum('legacy' in r[3].split(',') for r in rows))
print('Inventory SHA256:',hashlib.sha256((out/'retirement-inventory.tsv').read_bytes()).hexdigest())
