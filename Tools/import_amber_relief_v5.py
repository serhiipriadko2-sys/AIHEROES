"""Import V5 desktop/mobile relief without replacing the original SM_Island."""
import unreal,pathlib,json,hashlib
root=pathlib.Path(unreal.Paths.project_dir()).resolve()
folder=root/'ArtSource'/'AmberMarch'/'ReliefV5'
manifest=json.loads((folder/'asset_manifest.json').read_text(encoding='utf-8'))
items=manifest.get('assets',[])
expected={'SM_IslandReliefV5','SM_IslandReliefV5_Mobile'}
if len(items)!=2 or {a['name'] for a in items}!=expected:
    raise RuntimeError('Unexpected terrain inventory')
tools=unreal.AssetToolsHelpers.get_asset_tools()
records=[]
for item in items:
    name=item['name']
    source=folder/(name+'.fbx')
    if hashlib.sha256(source.read_bytes()).hexdigest()!=item['sha256']:
        raise RuntimeError('Source hash mismatch '+name)
    asset='/Game/AmberMarch/Meshes/'+name
    if unreal.EditorAssetLibrary.does_asset_exist(asset):
        raise RuntimeError('Asset already exists, no overwrite '+asset)
    task=unreal.AssetImportTask()
    task.filename=str(source)
    task.destination_path='/Game/AmberMarch/Meshes'
    task.destination_name=name
    task.automated=True;task.save=True;task.replace_existing=False
    opts=unreal.FbxImportUI()
    opts.import_mesh=True;opts.import_as_skeletal=False
    opts.import_materials=False;opts.import_textures=False;opts.import_animations=False
    opts.automated_import_should_detect_type=False
    opts.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
    opts.static_mesh_import_data.combine_meshes=True
    opts.static_mesh_import_data.auto_generate_collision=False
    task.options=opts
    tools.import_asset_tasks([task])
    mesh=unreal.load_asset(asset)
    if not isinstance(mesh,unreal.StaticMesh):
        raise RuntimeError('Import failed '+asset)
    slots=mesh.get_editor_property('static_materials')
    colors=[]
    for idx,slot in enumerate(slots):
        color=str(slot.get_editor_property('material_slot_name'))
        if color not in ('Grass','Moss','Stone','Path'):
            raise RuntimeError('Unknown material '+color+' in '+name)
        mat=unreal.load_asset('/Game/AmberMarch/Materials/M_'+color)
        if not isinstance(mat,unreal.MaterialInterface):
            raise RuntimeError('Missing material '+color)
        mesh.set_material(idx,mat)
        colors.append(color)
    if not unreal.EditorAssetLibrary.save_loaded_asset(mesh):
        raise RuntimeError('Save failed '+name)
    extent=float(mesh.get_bounds().box_extent.length())
    if not 2000<extent<3000:
        raise RuntimeError('Terrain size unexpected '+str(extent))
    records.append(dict(name=name,extent_cm=extent,materials=colors,path=asset))
out=root/'Saved'/'QA'/'relief_v5_import_receipt.json'
out.write_text(json.dumps(dict(status='PASS',assets=records),indent=2)+'\n',encoding='utf-8')
unreal.log('RELIEF_V5_IMPORT_PASS count=2')