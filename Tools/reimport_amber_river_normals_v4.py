"""Replace ONLY new V4 river mesh with inspected upward-facing Blender asset.
Use only after staging and backing up the original uasset.
"""
import unreal,json,hashlib,pathlib
root=pathlib.Path(unreal.Paths.project_dir()).resolve()
qa=root/'Saved'/'QA'/'RiverV4_20261029'
stage=qa/'fixed'
back=qa/'before_water_repair'
src=stage/'SM_Silverbrook.fbx'
manifest=json.loads((stage/'asset_manifest.json').read_text(encoding='utf-8'))
spec=next((a for a in manifest['assets'] if a['name']=='SM_Silverbrook'),None)
if not spec or hashlib.sha256(src.read_bytes()).hexdigest()!=spec['sha256']:
    raise RuntimeError('Fixed source checksum mismatch')
if not (back/'SM_Silverbrook.uasset').is_file():
    raise RuntimeError('Missing read-back backup, refuse reimport')
name='SM_Silverbrook';dest='/Game/AmberMarch/Meshes'
path=dest+'/'+name
if not unreal.EditorAssetLibrary.does_asset_exist(path):
    raise RuntimeError('Source game asset vanished')
task=unreal.AssetImportTask()
task.filename=str(src);task.destination_path=dest;task.destination_name=name
task.automated=True;task.save=True
task.replace_existing=True
opt=unreal.FbxImportUI()
opt.import_mesh=True;opt.import_as_skeletal=False
opt.import_materials=False;opt.import_textures=False;opt.import_animations=False
opt.automated_import_should_detect_type=False
opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
opt.static_mesh_import_data.combine_meshes=True
opt.static_mesh_import_data.auto_generate_collision=False
task.options=opt
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
mesh=unreal.load_asset(path)
if not isinstance(mesh,unreal.StaticMesh):raise RuntimeError('Reimport did not produce mesh')
slots=mesh.get_editor_property('static_materials')
colors=[]
for index,slot in enumerate(slots):
    color=str(slot.get_editor_property('material_slot_name'))
    mat=unreal.load_asset('/Game/AmberMarch/Materials/M_'+color)
    if not isinstance(mat,unreal.MaterialInterface):
        raise RuntimeError('Unknown palette entry '+color)
    mesh.set_material(index,mat)
    colors.append(color)
if not {'Water','Grass','Moss','Crystal'}.issubset(colors):
    raise RuntimeError('Unexpected slots missing after reimport')
if not unreal.EditorAssetLibrary.save_loaded_asset(mesh):
    raise RuntimeError('Failed to save corrected mesh')
receipt={'status':'PASS','path':path,'source_sha256':spec['sha256'],
         'material_slots':colors,'extent_cm':float(mesh.get_bounds().box_extent.length())}
(qa/'water_reimport.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
unreal.log('RIVER_V4_NORMALS_REIMPORT_PASS')