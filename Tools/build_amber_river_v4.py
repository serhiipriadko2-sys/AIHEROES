"""AIHEROES AmberMarch V4: original low-poly river and road crossing.
Blender 5.2 --background --python Tools/build_amber_river_v4.py
Visual scene dressing, never a source of pathfinding or collision.
"""
import bpy, math, json, hashlib, random
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'ArtSource'/'AmberMarch'/'RiverV4'
if OUT.exists() and any(OUT.iterdir()):
    raise RuntimeError('RiverV4 already exists; refusing overwrite')
OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
palette=json.loads((ROOT/'ArtSource'/'AmberMarch'/'palette.json').read_text(encoding='utf-8'))
materials={}
for key,color in palette.items():
    m=bpy.data.materials.new(key)
    m.diffuse_color=color
    m.use_nodes=True
    bs=m.node_tree.nodes.get('Principled BSDF')
    bs.inputs['Base Color'].default_value=color
    bs.inputs['Roughness'].default_value=.81
    materials[key]=m
parts=[]
inventory=[]
def add(o,mat):
    o.data.materials.append(materials[mat]);parts.append(o);return o
def box(name,p,dim,mat):
    bpy.ops.mesh.primitive_cube_add(size=1,location=p)
    o=bpy.context.object;o.name=name
    o.scale=dim
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    return add(o,mat)
def ico(name,p,dim,mat):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1,radius=1,location=p)
    o=bpy.context.object;o.name=name;o.scale=dim
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    return add(o,mat)
def cone(name,p,r1,r2,h,mat,n=6):
    bpy.ops.mesh.primitive_cone_add(vertices=n,radius1=r1,radius2=r2,depth=h,location=p)
    bpy.context.object.name=name
    return add(bpy.context.object,mat)
def ribbon(name,left,right,mat):
    if len(left)!=len(right) or len(left)<2:raise ValueError(name)
    vertices=[]
    for a,b in zip(left,right):vertices.extend([a,b])
    faces=[]
    for i in range(len(left)-1):
        v=2*i
        faces.append((v,v+2,v+3,v+1))
    mesh=bpy.data.meshes.new(name)
    mesh.from_pydata(vertices,[],faces);mesh.update()
    o=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(o)
    return add(o,mat)
def export(name):
    global parts
    if not parts:raise RuntimeError('Empty asset '+name)
    bpy.ops.object.select_all(action='DESELECT')
    for p in parts:p.select_set(True)
    bpy.context.view_layer.objects.active=parts[0]
    bpy.ops.object.join()
    o=bpy.context.object;o.name=name
    bpy.context.scene.cursor.location=(0,0,0)
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    o.data.calc_loop_triangles()
    tris=len(o.data.loop_triangles)
    if not 30<tris<5000:raise RuntimeError(name+' budget '+str(tris))
    file=OUT/(name+'.fbx')
    bpy.ops.export_scene.fbx(filepath=str(file),use_selection=True,object_types={'MESH'},
        apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',
        axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,
        use_triangles=True)
    if file.stat().st_size<1500:raise RuntimeError('Empty '+name)
    inventory.append({'name':name,'triangles':tris,'bytes':file.stat().st_size,
        'sha256':hashlib.sha256(file.read_bytes()).hexdigest()})
    parts=[]
    o.hide_set(True)
# Watercourse, not a landscape sculpt: all geometry sits on the island surface.
# Its axial direction is local X, its cross-section is local Y.
samples=25
X=[-5.6+i*(11.2/(samples-1)) for i in range(samples)]
def center(x):return .13*math.sin(x*.74)+.08*math.cos(1.47*x)
def width(x):return .64+.07*math.sin(.67*x+.4)
water_l=[];water_r=[]
bank_ll=[];bank_lr=[];bank_rl=[];bank_rr=[]
for x in X:
    c,w=center(x),width(x)
    water_l.append((x,c-w,.10))
    water_r.append((x,c+w,.10))
    bank_ll.append((x,c-w-.27,.14))
    bank_lr.append((x,c-w,.18))
    bank_rl.append((x,c+w,.18))
    bank_rr.append((x,c+w+.27,.14))
ribbon('BrookWater',water_l,water_r,'Water')
ribbon('BankLeft',bank_ll,bank_lr,'Moss')
ribbon('BankRight',bank_rl,bank_rr,'Grass')
# Flow lines break up a flat single-colour water plane without translucency.
for bias in (-.28,.29):
    for seg in (3,10,17):
        aa=[];bb=[]
        for k in range(seg,seg+4):
            x=X[k]
            y=center(x)+bias
            aa.append((x,y-.018,.118))
            bb.append((x,y+.018,.118))
        ribbon('CurrentFoam',aa,bb,'Crystal')
rng=random.Random(98231)
for i in range(28):
    x=rng.uniform(-5.3,5.3)
    side=1 if i%2 else -1
    y=center(x)+side*(width(x)+rng.uniform(.22,.48))
    stone=ico('RiverPebble',(x,y,.16),(.10+rng.random()*.11,.12+rng.random()*.10,.08+rng.random()*.08),'Stone')
for i in range(24):
    x=rng.uniform(-5.25,5.25)
    side=1 if i%2 else -1
    y=center(x)+side*(width(x)+rng.uniform(.40,.67))
    cone('Reed',(x,y,.21),.075,.017,.40,'Pine',6)
export('SM_Silverbrook')
# Stone footbridge: cross-stream direction local Y; centre at water crossing.
for j in range(15):
    y=-1.6+j*(3.2/14)
    z=.16+.17*(1-(y/1.6)**2)
    slab=box('BridgePaving',(0,y,z+.045),(1.72,.235,.15),'Ivory' if j%5 else 'Stone')
    if j in (0,14):
        box('BankAbutment',(0,y,.13),(1.95,.48,.26),'Stone')
# Low masonry rail sides. Posts, not collision walls.
for x in (-.93,.93):
    for j in range(7):
        y=-1.47+j*.49
        z=.28+.17*(1-(y/1.6)**2)
        box('BridgePost',(x,y,z+.36),(.19,.20,.73),'Stone')
    for j in range(6):
        y=-1.225+j*.49
        z=.28+.17*(1-(y/1.6)**2)
        box('BridgeRail',(x,y,z+.68),(.16,.50,.15),'Ivory')
for y in (-1.0,1.0):
    for x in (-.58,.58):
        box('Pier',(x,y,.02),(.25,.30,.22),'Stone')
export('SM_BrookBridge')
for o in bpy.data.objects:o.hide_set(False)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'RiverV4.blend'))
info={'origin':'Original procedural geometry, no reference image pixels',
      'authority':'Decorative and non-colliding; must follow verified HQ-FOREST road geometry',
      'assets':inventory}
(OUT/'asset_manifest.json').write_text(json.dumps(info,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('RIVER_V4_BUILD_PASS assets='+str(len(inventory))+
      ' triangles='+str(sum(x['triangles'] for x in inventory)))