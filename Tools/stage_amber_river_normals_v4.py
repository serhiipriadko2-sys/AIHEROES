"""Stage a corrected Blender river with upward-facing water/bank ribbons.
Does NOT overwrite any original FBX, blend or uasset.
"""
import bpy,bmesh,json,hashlib,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/'ArtSource'/'AmberMarch'/'RiverV4'
STAGED=ROOT/'Saved'/'QA'/'RiverV4_20261029'/'fixed'
if STAGED.exists() and any(STAGED.iterdir()):
    raise RuntimeError('Fixed stage already exists; refuse overwrite')
STAGED.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(SRC/'RiverV4.blend'))
obj=bpy.data.objects.get('SM_Silverbrook')
if not obj:raise RuntimeError('Source mesh missing')
bm=bmesh.new();bm.from_mesh(obj.data)
names={i:slot.material.name for i,slot in enumerate(obj.material_slots)}
target={'Water','Moss','Grass','Crystal'}
to_flip=[face for face in bm.faces
    if names.get(face.material_index) in target and face.normal.z<-.8]
if len(to_flip)!=90:
    raise RuntimeError('Unexpected fix set; expected exactly 90 lower-facing water/bank faces, got '+str(len(to_flip)))
bmesh.ops.reverse_faces(bm,faces=to_flip)
bm.to_mesh(obj.data);bm.free();obj.data.update()
down=sum(1 for p in obj.data.polygons
         if names.get(p.material_index) in target and p.normal.z<-.8)
if down:
    raise RuntimeError('Corrected ribbons still face downward: '+str(down))
bpy.ops.wm.save_as_mainfile(filepath=str(STAGED/'RiverV4.blend'))
bpy.ops.object.select_all(action='DESELECT')
obj.select_set(True);bpy.context.view_layer.objects.active=obj
output=STAGED/'SM_Silverbrook.fbx'
bpy.ops.export_scene.fbx(filepath=str(output),use_selection=True,object_types={'MESH'},
    apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',
    axis_forward='-Y',axis_up='Z',bake_anim=False,
    add_leaf_bones=False,use_triangles=True)
if output.stat().st_size<20000:
    raise RuntimeError('Truncated repair FBX')
shutil.copy2(SRC/'SM_BrookBridge.fbx',STAGED/'SM_BrookBridge.fbx')
manifest=json.loads((SRC/'asset_manifest.json').read_text(encoding='utf-8'))
for spec in manifest['assets']:
    file=STAGED/(spec['name']+'.fbx')
    spec['sha256']=hashlib.sha256(file.read_bytes()).hexdigest()
    spec['bytes']=file.stat().st_size
(STAGED/'asset_manifest.json').write_text(
    json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('RIVER_V4_NORMAL_REPAIR_STAGED flipped='+str(len(to_flip))+
      ' bytes='+str(output.stat().st_size))