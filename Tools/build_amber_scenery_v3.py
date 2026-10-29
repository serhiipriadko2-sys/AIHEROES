"""Original lightweight scenic meshes for AIHEROES: noninteractive terrain dressing.
Blender 5.x --background --python Tools/build_amber_scenery_v3.py
Refuses to overwrite an existing scenery kit.
"""
import bpy, math, random, json, hashlib
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'ArtSource'/'AmberMarch'/'SceneryV3'
if OUT.exists() and any(OUT.iterdir()):
    raise RuntimeError('SceneryV3 already exists; inspect before regenerating')
OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system='METRIC'
PAL=json.loads((ROOT/'ArtSource'/'AmberMarch'/'palette.json').read_text())
MATS={}
for name,rgba in PAL.items():
    m=bpy.data.materials.new(name)
    m.diffuse_color=rgba
    m.use_nodes=True
    m.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value=rgba
    m.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value=.82
    MATS[name]=m
parts=[]
assets=[]
def keep(obj,name,mat):
    obj.name=name
    obj.data.materials.append(MATS[mat])
    parts.append(obj)
    return obj
def block(name,xyz,dims,mat):
    bpy.ops.mesh.primitive_cube_add(size=1,location=xyz)
    o=bpy.context.object
    o.scale=dims
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    return keep(o,name,mat)
def cone(name,xyz,rb,rt,height,mat,verts=8):
    bpy.ops.mesh.primitive_cone_add(vertices=verts,radius1=rb,radius2=rt,depth=height,location=xyz)
    return keep(bpy.context.object,name,mat)
def sphere(name,xyz,scale,mat,subdiv=1):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=subdiv,radius=1,location=xyz)
    o=bpy.context.object
    o.scale=scale
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    return keep(o,name,mat)
def beam(name,pa,pb,width,mat):
    a,b=Vector(pa),Vector(pb)
    o=block(name,(a+b)*.5,(width,width,(b-a).length),mat)
    o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler()
    return o
def export(name):
    global parts
    if not parts: raise RuntimeError('Empty scene '+name)
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts:o.select_set(True)
    bpy.context.view_layer.objects.active=parts[0]
    bpy.ops.object.join()
    o=bpy.context.object
    o.name=name
    bpy.context.scene.cursor.location=(0,0,0)
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    o.data.calc_loop_triangles()
    tris=len(o.data.loop_triangles)
    if not 20<tris<12000:raise RuntimeError('Geometry budget '+name+': '+str(tris))
    path=OUT/(name+'.fbx')
    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},
        apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',
        bake_anim=False,add_leaf_bones=False,use_triangles=True)
    if path.stat().st_size<1500:raise RuntimeError('Empty FBX '+name)
    assets.append({'name':name,'triangles':tris,'bytes':path.stat().st_size,
                   'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
    parts=[]
    o.hide_set(True)
# Faceted high outcrop: distinct triangular strata and mossy crowns.
rng=random.Random(23019)
for cx,cy,rx,ry,h in ((-1.8,-.5,3.0,2.7,3.7),(1.8,.8,2.7,2.4,4.5)):
    count=11
    verts=[]
    for ring,zmul,rad in ((0,-.08,1.20),(1,.37,1.0),(2,.82,.71),(3,1.0,.28)):
        for j in range(count):
            theta=2*math.pi*j/count
            rr=rad*rng.uniform(.82,1.13)
            verts.append((cx+rx*rr*math.cos(theta),
                          cy+ry*rr*math.sin(theta),
                          h*zmul + rng.uniform(-.08,.08)))
    faces=[]
    for ring in range(3):
        for j in range(count):
            k=ring*count+j; nn=ring*count+(j+1)%count
            faces.extend(((k,nn,nn+count),(k,nn+count,k+count)))
    faces.append(tuple((3*count)+j for j in range(count)))
    mesh=bpy.data.meshes.new('Strata')
    mesh.from_pydata(verts,[],faces);mesh.update()
    o=bpy.data.objects.new('Strata',mesh)
    bpy.context.collection.objects.link(o)
    keep(o,'CliffStrata','Stone')
    o.data.materials.append(MATS['Moss'])
    for face in o.data.polygons:
        if face.index>=2*count*2:face.material_index=1
for x,y,z in ((-.6,-1.4,2.7),(-2.4,.3,3.3),(2.5,.1,3.7)):
    sphere('CliffLichen',(x,y,z),(.5,.55,.16),'Moss')
export('SM_RidgeOutcrop')
# Civic road beacon: a landmark, not a travel gate.
cone('Paving',(0,0,.12),1.10,1.1,.24,'Stone',10)
cone('Base',(0,0,.42),.72,.58,.42,'Ivory',8)
block('Obelisk',(0,0,1.52),(.68,.68,2.20),'Stone')
cone('Crown',(0,0,2.78),.62,.25,.55,'Bronze',8)
cone('SignalPrism',(0,0,3.20),.28,0,.72,'Crystal',6)
for z in (.76,1.94,2.64):
    block('Band',(0,0,z),(.82,.82,.10),'Bronze')
for x in (-.15,.15):
    block('Glyph',(x,-.358,1.53),(.07,.025,.42),'Amber')
export('SM_Waystone')
# Small occupied waystation: equipment next to the citadel, not a gameplay node.
block('CaravanDeck',(0,0,.52),(2.9,1.65,.26),'Bark')
for x in (-1.02,1.02):
    for y in (-.92,.92):
        wheel=cone('CartWheel',(x,y,.48),.38,.38,.19,'Bronze',12)
        wheel.rotation_euler.x=math.pi/2
for x in (-1.05,1.05):
    for y in (-.65,.65):
        beam('CanopySupport',(x,y,.65),(x,y,2.24),.10,'Bark')
block('Canopy',(0,0,2.35),(3.14,1.87,.12),'Slate')
for x in (-.85,-.18,.58):
    block('Crate',(x,.06,1.05),(.57,.63,.72),'Bark')
    block('CrateBand',(x,.06,1.38),(.60,.66,.075),'Bronze')
for y in (-.35,.35):
    cone('CargoBarrel',(1.25,y,1.0),.25,.25,.7,'Bronze',10)
beam('TowBeam',(1.3,0,.55),(3.2,0,.43),.13,'Bark')
export('SM_Waystation')
for o in bpy.data.objects:o.hide_set(False)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'SceneryV3.blend'))
receipt={'origin':'Original, procedural meshes; no extracted reference-image pixels',
    'purpose':'Visual-only, collisions off, no road/path authority',
    'assets':assets}
(OUT/'asset_manifest.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
print('SCENERY_V3_BUILD_PASS count='+str(len(assets))+
      ' tris='+str(sum(x['triangles'] for x in assets)))