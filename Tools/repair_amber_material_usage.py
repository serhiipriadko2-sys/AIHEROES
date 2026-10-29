"""Enable instanced foliage usage on the three newly authored materials only."""
import unreal,json,hashlib,shutil
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
out=root/'Saved/QA/VisualDirection_20261029T140048/material_usage'
out.mkdir(parents=True,exist_ok=True)
records=[]
for name in ('Bark','Pine','Leaf'):
    file=root/f'Content/AmberMarch/Materials/M_{name}.uasset'
    mat=unreal.load_asset(f'/Game/AmberMarch/Materials/M_{name}')
    if not isinstance(mat,unreal.Material):raise RuntimeError('Material missing '+name)
    before=hashlib.sha256(file.read_bytes()).hexdigest()
    backup=out/file.name
    if backup.exists():raise RuntimeError('Existing material baseline; inspect first')
    shutil.copy2(file,backup)
    usage=unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES
    was=unreal.MaterialEditingLibrary.has_material_usage(mat,usage)
    unreal.MaterialEditingLibrary.set_base_material_usage(mat,usage,True)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    if not unreal.EditorAssetLibrary.save_loaded_asset(mat):raise RuntimeError('Save failed '+name)
    if not unreal.MaterialEditingLibrary.has_material_usage(mat,usage):raise RuntimeError('Readback failed '+name)
    records.append({'material':name,'was_enabled':was,'enabled':True,'before_sha256':before,'after_sha256':hashlib.sha256(file.read_bytes()).hexdigest()})
(out/'receipt.json').write_text(json.dumps(records,indent=2)+'\n')
unreal.log('AMBER_MATERIAL_USAGE_PASS count=3')
