"""Import original AmberVanguard meshes and same-mesh portraits into new packages only."""
import unreal, json, hashlib
from pathlib import Path
ROOT=Path(unreal.Paths.project_dir()).resolve()
SRC=ROOT/'ArtSource'/'AmberVanguard'; DEST='/Game/AmberMarch/Units'
manifest=json.loads((SRC/'asset_manifest.json').read_text('utf-8'))
palette=json.loads((SRC/'palette.json').read_text('utf-8'))
at=unreal.AssetToolsHelpers.get_asset_tools()
# Preflight all destinations before writing any package.
for item in manifest['assets']:
    for prefix in ('SM_','T_'):
        path=DEST+'/'+prefix+item['unit']
        if unreal.EditorAssetLibrary.does_asset_exist(path):raise RuntimeError('Existing destination: '+path)
    for prefix,ext,key in [('SM_','.fbx','fbx_sha256'),('T_','.png','portrait_sha256')]:
        f=SRC/(prefix+item['unit']+ext)
        if hashlib.sha256(f.read_bytes()).hexdigest()!=item[key]:raise RuntimeError('Source changed: '+str(f))
materials={}
for name,color in palette.items():
    path=DEST+'/Materials/UM_'+name
    if unreal.EditorAssetLibrary.does_asset_exist(path):raise RuntimeError('Existing material: '+path)
    m=at.create_asset('UM_'+name,DEST+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
    v=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionVectorParameter,-400,0)
    v.set_editor_property('parameter_name','Tint');v.set_editor_property('default_value',unreal.LinearColor(*color))
    unreal.MaterialEditingLibrary.connect_material_property(v,'',unreal.MaterialProperty.MP_BASE_COLOR)
    r=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant,-400,150)
    r.set_editor_property('r',.72 if name=='Bronze' else .9)
    unreal.MaterialEditingLibrary.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
    e=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionMultiply,-130,280)
    e.set_editor_property('const_b',.12 if name!='Amber' else .65)
    unreal.MaterialEditingLibrary.connect_material_expressions(v,'',e,'A')
    unreal.MaterialEditingLibrary.connect_material_property(e,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m);materials[name]=m
records=[]
for item in manifest['assets']:
    name='SM_'+item['unit']; task=unreal.AssetImportTask()
    task.filename=str(SRC/(name+'.fbx'));task.destination_path=DEST;task.destination_name=name
    task.automated=True;task.save=True;task.replace_existing=False
    opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_as_skeletal=False
    opt.import_materials=False;opt.import_textures=False;opt.import_animations=False
    opt.automated_import_should_detect_type=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
    opt.static_mesh_import_data.combine_meshes=True;opt.static_mesh_import_data.auto_generate_collision=False
    task.options=opt;at.import_asset_tasks([task])
    mesh=unreal.load_asset(DEST+'/'+name)
    if not isinstance(mesh,unreal.StaticMesh):raise RuntimeError('Missing mesh '+name)
    for i,slot in enumerate(mesh.get_editor_property('static_materials')):
        slotname=str(slot.get_editor_property('material_slot_name'))
        if slotname not in materials:raise RuntimeError('Unmapped slot '+slotname)
        mesh.set_material(i,materials[slotname])
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    t=unreal.AssetImportTask();t.filename=str(SRC/('T_'+item['unit']+'.png'))
    t.destination_path=DEST;t.destination_name='T_'+item['unit'];t.automated=True;t.save=True;t.replace_existing=False
    at.import_asset_tasks([t]);texture=unreal.load_asset(DEST+'/T_'+item['unit'])
    if not isinstance(texture,unreal.Texture2D):raise RuntimeError('Missing portrait')
    texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('srgb',True);unreal.EditorAssetLibrary.save_loaded_asset(texture)
    b=mesh.get_bounds()
    if not 90 < b.box_extent.z*2 < 230:raise RuntimeError('Unexpected imported scale '+name)
    records.append({'unit':item['unit'],'mesh':DEST+'/'+name,'portrait':DEST+'/T_'+item['unit'],
        'extent':[b.box_extent.x,b.box_extent.y,b.box_extent.z],'triangles_source':item['triangles']})
    unreal.log('AMBER_UNIT_IMPORTED '+name)
out=ROOT/'Saved'/'QA'/'BattleArtWork_20261029'/'import_receipt.json'
if out.exists():raise RuntimeError('Import receipt already exists')
out.write_text(json.dumps({'status':'PASS_IMPORTED_UNITS','count':len(records),'assets':records},indent=2)+'\n',encoding='utf-8')
unreal.log('AMBER_UNITS_IMPORT_PASS count='+str(len(records)))
