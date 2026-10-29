"""Enable instancing only on the original AmberMarch stone material."""
import hashlib, json
from pathlib import Path
import unreal
root = Path(unreal.Paths.project_dir()).resolve()
evidence = root / 'Saved/QA/ReferenceVisual_20261029'
path = root / 'Content/AmberMarch/Materials/M_Stone.uasset'
baseline = json.loads((evidence / 'before.json').read_text('utf-8-sig'))
expected = next(x['sha256'].lower() for x in baseline if x['path'].endswith('M_Stone.uasset'))
before = hashlib.sha256(path.read_bytes()).hexdigest()
mat = unreal.load_asset('/Game/AmberMarch/Materials/M_Stone')
if not isinstance(mat, unreal.Material):
    raise RuntimeError('Original stone material unavailable')
usage = unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES
was = unreal.MaterialEditingLibrary.has_material_usage(mat, usage)
if not was:
    if before != expected:
        raise RuntimeError('Stone changed since backup; inspect before modifying')
    unreal.MaterialEditingLibrary.set_base_material_usage(mat, usage, True)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    if not unreal.EditorAssetLibrary.save_loaded_asset(mat):
        raise RuntimeError('Material save failed')
if not unreal.MaterialEditingLibrary.has_material_usage(mat, usage):
    raise RuntimeError('Instancing read-back failed')
record = {'status':'PASS', 'before_sha256':before,
          'after_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
          'already_enabled':was, 'material':'M_Stone'}
(evidence / 'stone-usage.json').write_text(json.dumps(record, indent=2)+'\n')
unreal.log('REFERENCE_STONE_USAGE_PASS')
