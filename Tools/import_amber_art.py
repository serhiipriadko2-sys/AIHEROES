"""Editor-only import of original AmberMarch FBX sources. No gameplay state writes."""
import unreal, json
from pathlib import Path
ROOT=Path(unreal.Paths.project_dir()).resolve()
SRC=ROOT/'ArtSource'/'AmberMarch'; DEST='/Game/AmberMarch'
at=unreal.AssetToolsHelpers.get_asset_tools()
manifest=json.loads((SRC/'asset_manifest.json').read_text())
palette=json.loads((SRC/'palette.json').read_text())
materials={}
for name,color in palette.items():
    path=DEST+'/Materials/M_'+name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        materials[name]=unreal.load_asset(path);continue
    mat=at.create_asset('M_'+name,DEST+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
    vec=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionVectorParameter,-350,0)
    vec.set_editor_property('parameter_name','Tint')
    vec.set_editor_property('default_value',unreal.LinearColor(*color))
    unreal.MaterialEditingLibrary.connect_material_property(vec,'',unreal.MaterialProperty.MP_BASE_COLOR)
    rough=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant,-350,140)
    rough.set_editor_property('r',0.88)
    unreal.MaterialEditingLibrary.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
    # A restrained fill prevents the original black silhouettes in the prototype.
    mul=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionMultiply,-100,260)
    mul.set_editor_property('const_b',0.18 if name!='Amber' else 1.5)
    unreal.MaterialEditingLibrary.connect_material_expressions(vec,'',mul,'A')
    unreal.MaterialEditingLibrary.connect_material_property(mul,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if name in ('Bark','Pine','Leaf'):
        unreal.MaterialEditingLibrary.set_base_material_usage(mat,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,True)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat);materials[name]=mat
records=[]
for item in manifest['assets']:
    name=item['name'];path=DEST+'/Meshes/'+name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        raise RuntimeError('Refuse existing mesh overwrite: '+path)
    task=unreal.AssetImportTask();task.filename=str(SRC/(name+'.fbx'))
    task.destination_path=DEST+'/Meshes';task.destination_name=name
    task.automated=True;task.save=True;task.replace_existing=False
    opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_as_skeletal=False
    opt.import_materials=False;opt.import_textures=False;opt.import_animations=False
    opt.automated_import_should_detect_type=False
    opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
    opt.static_mesh_import_data.combine_meshes=True
    opt.static_mesh_import_data.auto_generate_collision=False
    opt.static_mesh_import_data.import_uniform_scale=1.0
    task.options=opt
    at.import_asset_tasks([task])
    mesh=unreal.load_asset(path)
    if not isinstance(mesh,unreal.StaticMesh):raise RuntimeError('Mesh missing '+path)
    slots=mesh.get_editor_property('static_materials')
    for i,slot in enumerate(slots):
        source=str(slot.get_editor_property('material_slot_name'))
        found=materials.get(source)
        if found is None:raise RuntimeError('Unknown palette slot '+source)
        mesh.set_material(i,found)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    bounds=mesh.get_bounds()
    records.append({'asset':path,'slots':len(slots),'extent':[bounds.box_extent.x,bounds.box_extent.y,bounds.box_extent.z]})
    unreal.log('AMBER_IMPORTED '+path)
(ROOT/'Saved'/'QA'/'VisualDirection_20261029T140048'/'import_receipt.json').write_text(json.dumps(records,indent=2)+'\n')
unreal.log('AMBER_IMPORT_PASS count='+str(len(records)))
