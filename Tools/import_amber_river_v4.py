"""Import two authored river-crossing meshes; no overwrite of existing assets."""
import unreal, pathlib, json, hashlib
root=pathlib.Path(unreal.Paths.project_dir()).resolve()
source=root/'ArtSource'/'AmberMarch'/'RiverV4'
manifest=json.loads((source/'asset_manifest.json').read_text(encoding='utf-8'))
whitelist={'SM_Silverbrook','SM_BrookBridge'}
assets=manifest.get('assets',[])
if {a['name'] for a in assets}!=whitelist or len(assets)!=2:
    raise RuntimeError('Unexpected V4 inventory')
dest='/Game/AmberMarch/Meshes'
toolkit=unreal.AssetToolsHelpers.get_asset_tools()
receipts=[]
for spec in assets:
    name=spec['name']
    file=source/(name+'.fbx')
    if hashlib.sha256(file.read_bytes()).hexdigest()!=spec['sha256']:
        raise RuntimeError('Source checksum drift: '+name)
    game=dest+'/'+name
    if unreal.EditorAssetLibrary.does_asset_exist(game):
        raise RuntimeError('Refusing existing mesh: '+game)
    task=unreal.AssetImportTask()
    task.filename=str(file);task.destination_path=dest;task.destination_name=name
    task.automated=True;task.save=True;task.replace_existing=False
    opt=unreal.FbxImportUI()
    opt.import_mesh=True;opt.import_as_skeletal=False
    opt.import_materials=False;opt.import_textures=False;opt.import_animations=False
    opt.automated_import_should_detect_type=False
    opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
    opt.static_mesh_import_data.combine_meshes=True
    opt.static_mesh_import_data.auto_generate_collision=False
    task.options=opt
    toolkit.import_asset_tasks([task])
    mesh=unreal.load_asset(game)
    if not isinstance(mesh,unreal.StaticMesh):
        raise RuntimeError('No imported mesh '+name)
    slots=mesh.get_editor_property('static_materials')
    for idx,slot in enumerate(slots):
        color=str(slot.get_editor_property('material_slot_name'))
        material=unreal.load_asset('/Game/AmberMarch/Materials/M_'+color)
        if not isinstance(material,unreal.MaterialInterface):
            raise RuntimeError('Unknown art material '+name+':'+color)
        mesh.set_material(idx,material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(mesh):
        raise RuntimeError('Cannot save '+name)
    bounds=mesh.get_bounds()
    extent=float(bounds.box_extent.length())
    if not 100.0<extent<1500.0:
        raise RuntimeError('Unexpected scale '+name+':'+str(extent))
    receipts.append({'name':name,'game_path':game,'extent_cm':extent,'slots':len(slots)})
out=root/'Saved'/'QA'/'river_v4_import_receipt.json'
out.write_text(json.dumps({'status':'PASS','assets':receipts},indent=2)+'\n',encoding='utf-8')
unreal.log('RIVER_V4_IMPORT_PASS count='+str(len(receipts)))