"""Editor-only import of three original SceneryV3 meshes; permits the recorded partial first import."""
import unreal, pathlib, json, hashlib
ROOT=pathlib.Path(unreal.Paths.project_dir()).resolve()
SRC=ROOT/'ArtSource'/'AmberMarch'/'SceneryV3'
records=[]
manifest=json.loads((SRC/'asset_manifest.json').read_text(encoding='utf-8'))
if len(manifest.get('assets',[]))!=3:raise RuntimeError('Scenery manifest count')
dest='/Game/AmberMarch/Meshes'
at=unreal.AssetToolsHelpers.get_asset_tools()
for spec in manifest['assets']:
    name=spec['name']
    if name not in ('SM_RidgeOutcrop','SM_Waystone','SM_Waystation'):
        raise RuntimeError('Unexpected mesh name')
    source=SRC/(name+'.fbx')
    if hashlib.sha256(source.read_bytes()).hexdigest()!=spec['sha256']:
        raise RuntimeError('FBX hash mismatch: '+name)
    path=dest+'/'+name
    existing=unreal.EditorAssetLibrary.does_asset_exist(path)
    # RidgeOutcrop was imported by this exact script before a Vector.size API error.
    if existing and name!='SM_RidgeOutcrop':
        raise RuntimeError('Refuse to overwrite pre-existing asset '+path)
    if not existing:
        task=unreal.AssetImportTask()
        task.filename=str(source);task.destination_path=dest
        task.destination_name=name;task.automated=True
        task.save=True;task.replace_existing=False
        opt=unreal.FbxImportUI()
        opt.import_mesh=True;opt.import_as_skeletal=False
        opt.import_materials=False;opt.import_textures=False
        opt.import_animations=False
        opt.automated_import_should_detect_type=False
        opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
        opt.static_mesh_import_data.combine_meshes=True
        opt.static_mesh_import_data.auto_generate_collision=False
        opt.static_mesh_import_data.import_uniform_scale=1.0
        task.options=opt
        at.import_asset_tasks([task])
    mesh=unreal.load_asset(path)
    if not isinstance(mesh,unreal.StaticMesh):raise RuntimeError('Missing import '+name)
    slots=mesh.get_editor_property('static_materials')
    for index,slot in enumerate(slots):
        mat_name=str(slot.get_editor_property('material_slot_name'))
        mat=unreal.load_asset('/Game/AmberMarch/Materials/M_'+mat_name)
        if not isinstance(mat,unreal.MaterialInterface):
            raise RuntimeError('Unmapped material '+mat_name+' for '+name)
        mesh.set_material(index,mat)
    if not unreal.EditorAssetLibrary.save_loaded_asset(mesh):
        raise RuntimeError('Cannot save '+name)
    b=mesh.get_bounds()
    ext=float(b.box_extent.length())
    if not 30.0<ext<3000.0:raise RuntimeError('Unreasonable world scale '+name+' '+str(ext))
    records.append(dict(name=name,path=path,extent_cm=ext,material_slots=len(slots)))
out=ROOT/'Saved'/'QA'/'scenery_v3_import_receipt.json'
out.write_text(json.dumps({'status':'PASS','assets':records},indent=2)+'\n',encoding='utf-8')
unreal.log('SCENERY_V3_IMPORT_PASS '+str(len(records)))