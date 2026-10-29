"""Original Amber March art kit. Run with Blender --background --python.
Writes only ArtSource/AmberMarch; never changes gameplay or user Blender files.
"""
import bpy, math, random, json, hashlib
from pathlib import Path
from mathutils import Vector
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'ArtSource' / 'AmberMarch'
OUT.mkdir(parents=True, exist_ok=True)
if (OUT / 'AmberMarch.blend').exists():
    raise RuntimeError('Art source already exists; inspect before regenerating')
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system='METRIC'
PALETTE = {'Stone':(0.43,0.47,0.43,1),'Ivory':(0.72,0.70,0.56,1),
 'Slate':(0.10,0.19,0.23,1),'Bronze':(0.48,0.27,0.10,1),'Amber':(1.0,0.53,0.13,1),
 'Pine':(0.09,0.26,0.19,1),'Leaf':(0.18,0.37,0.24,1),'Bark':(0.23,0.14,0.09,1),
 'Grass':(0.31,0.40,0.25,1),'Moss':(0.25,0.34,0.22,1),'Water':(0.08,0.24,0.27,1),
 'Path':(0.52,0.43,0.28,1),'Dark':(0.055,0.07,0.065,1),'Crystal':(0.20,0.61,0.61,1)}
MATS={}
for name,color in PALETTE.items():
    m=bpy.data.materials.new(name); m.diffuse_color=color; m.use_nodes=True
    bs=m.node_tree.nodes.get('Principled BSDF'); bs.inputs['Base Color'].default_value=color
    bs.inputs['Roughness'].default_value=0.85
    MATS[name]=m
objects=[]; stats=[]
def finish(o,name,mat):
    o.name=name; o.data.materials.append(MATS[mat]); objects.append(o); return o
def box(name,at,size,mat,rot=(0,0,0)):
    bpy.ops.mesh.primitive_cube_add(size=1,location=at); o=bpy.context.object
    o.scale=size; o.rotation_euler=rot; bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    return finish(o,name,mat)
def cone(name,at,r1,r2,h,mat,n=8):
    bpy.ops.mesh.primitive_cone_add(vertices=n,radius1=r1,radius2=r2,depth=h,location=at)
    return finish(bpy.context.object,name,mat)
def ico(name,at,size,mat):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1,radius=1,location=at)
    o=bpy.context.object; o.scale=size; bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    return finish(o,name,mat)
def beam(name,a,b,width,mat):
    a,b=Vector(a),Vector(b); o=box(name,(a+b)*.5,(width,width,(b-a).length),mat)
    o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler(); return o
def roof(at,w,d,h):
    x,y,z=at
    verts=[(-w/2,-d/2,0),(w/2,-d/2,0),(w/2,d/2,0),(-w/2,d/2,0),(0,-d/2,h),(0,d/2,h)]
    mesh=bpy.data.meshes.new('GabledRoof');mesh.from_pydata(verts,[],[(0,1,4),(3,5,2),(0,4,5,3),(1,2,5,4),(0,3,2,1)])
    mesh.update();o=bpy.data.objects.new('Roof',mesh);bpy.context.collection.objects.link(o);o.location=(x,y,z);finish(o,'Roof','Slate')
def pine(x,y,s=1):
    cone('Trunk',(x,y,.9*s),.11*s,.08*s,1.8*s,'Bark')
    for z,r,h in [(1.5,.9,1.5),(2.15,.66,1.4),(2.75,.4,1.1)]:
        cone('Crown',(x,y,z*s),r*s,0,h*s,'Pine' if z!=2.15 else 'Leaf')
def export(name):
    global objects
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects:o.select_set(True)
    bpy.context.view_layer.objects.active=objects[0];bpy.ops.object.join();o=bpy.context.object
    o.name=name;bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    o.data.calc_loop_triangles();tris=len(o.data.loop_triangles)
    f=OUT/(name+'.fbx')
    bpy.ops.export_scene.fbx(filepath=str(f),use_selection=True,object_types={'MESH'},
        apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',
        bake_anim=False,add_leaf_bones=False,use_triangles=True)
    stats.append({'name':name,'triangles':tris,'bytes':f.stat().st_size,'sha256':hashlib.sha256(f.read_bytes()).hexdigest()})
    objects=[]
    o.hide_set(True)
    return o
# Headquarters: crenellated observatory enclosing an amber cognition core.
cone('Foundation',(0,0,.16),2.4,2.4,.32,'Stone',12)
cone('Keep',(0,0,1.25),1.35,1.35,2.2,'Ivory',8)
cone('CopperBelt',(0,0,2.3),1.5,1.5,.18,'Bronze',8)
cone('Roof',(0,0,2.8),1.7,.45,1.0,'Slate',8)
for x,y in [(-1.6,-1.5),(-1.6,1.5),(1.6,-1.5),(1.6,1.5)]:
    cone('Tower',(x,y,1.25),.48,.48,2.5,'Stone',8)
    cone('TowerRoof',(x,y,2.7),.65,0,.9,'Slate',8)
for x in [-1,0,1]:box('EntrySteps',(-1.8+x*.15,0,.12+x*.02),(.65,1.0,.15),'Ivory')
box('Door',(-1.37,0,.85),(.08,.6,1.5),'Dark')
ico('Core',(0,0,3.45),(.43,.43,.65),'Amber')
for a in range(0,360,90):
    r=math.radians(a);beam('CoreProng',(math.cos(r)*.8,math.sin(r)*.8,2.8),(math.cos(r)*.55,math.sin(r)*.55,3.8),.1,'Bronze')
export('SM_Hearth')
# Sawmill: timber hall, raised roof, stacks and a large saw wheel.
box('Hall',(0,0,.85),(2.6,1.8,1.7),'Ivory');roof((0,0,1.7),3.1,2.25,1)
for x in [-1.3,0,1.3]:box('TimberPost',(x,-.94,.85),(.12,.12,1.8),'Bark')
for z in [.15,1.6]:box('TimberRail',(0,-.97,z),(2.8,.12,.12),'Bark')
for i in range(5):
    o=cone('Log',(-1.5+i*.28,1.5,.35),.22,.22,1.8,'Bark');o.rotation_euler.y=math.pi/2
for i in range(10):
    a=i*math.tau/10;beam('WheelSpoke',(1.65,-.4,1.2),(1.65,-.4+math.cos(a)*.7,1.2+math.sin(a)*.7),.09,'Bronze')
pine(-2.1,1,0.95);pine(1.6,1.5,.8)
export('SM_Sawmill')
# Quarry: recognisable dark entrance, braces, stone and ore.
for at,sz in [((0,.5,.7),(1.6,1.2,1.1)),((-1.2,.2,.55),(.9,.85,.9)),((1.2,.6,.8),(1,1,1.3))]:ico('Rock',at,sz,'Stone')
box('MineOpening',(-.2,-.65,.65),(1.15,.13,1.25),'Dark')
for x in [-.85,.45]:box('Brace',(x,-.8,.75),(.15,.2,1.5),'Bark')
box('Lintel',(-.2,-.82,1.5),(1.5,.23,.2),'Bark')
for y in [-1.5,-1.1,-.7]:box('RailSleeper',(-.2,y,.08),(1.1,.12,.12),'Bark')
for x in [-.55,.15]:box('Rail',(x,-1.15,.15),(.06,1.25,.08),'Bronze')
for x,y,z in [(1,-.7,.5),(1.5,-.5,.55),(.75,-.4,.55)]:cone('OreCrystal',(x,y,z),.22,0,.8,'Crystal',5)
export('SM_Quarry')
# Outpost: palisade and watchtower.
cone('TowerBase',(0,0,1.1),.8,.7,2.2,'Stone',8)
box('Platform',(0,0,2.2),(2.1,2.1,.2),'Bark')
for x in [-.8,.8]:
 for y in [-.8,.8]:beam('Column',(x,y,2.2),(x,y,3.3),.13,'Bark')
roof((0,0,3.3),2.4,2.4,.8)
for i in range(12):
 a=(i/12)*math.tau
 if math.cos(a)<-.8:continue
 cone('Palisade',(math.cos(a)*1.65,math.sin(a)*1.65,.7),.13,.1,1.4,'Bark',6)
beam('Flagpole',(.85,.85,3.3),(.85,.85,4.3),.05,'Bronze')
box('Banner',(.85,1.13,3.95),(.03,.5,.6),'Amber')
export('SM_Watchtower')
# Archive: broken concentric columns and ancient data prism.
cone('Dais',(0,0,.2),2.2,2.2,.4,'Stone',10)
for i in range(6):
 a=i*math.tau/6;h=1.8 if i in [2,3] else 3.0
 cone('ArchiveColumn',(math.cos(a)*1.6,math.sin(a)*1.6,h*.5+.35),.23,.23,h,'Ivory',6)
 if h>2:box('Capital',(math.cos(a)*1.6,math.sin(a)*1.6,h+.35),(.65,.65,.22),'Bronze')
cone('CorePlinth',(0,0,.7),.8,.6,.8,'Slate',8)
ico('MemoryPrism',(0,0,1.85),(.55,.55,1.05),'Crystal')
for i in range(4):ico('Rubble',(-1.8+i*.7,-1.7,.3),(.35,.45,.3),'Stone')
export('SM_Archive')
# Hero: a hooded bronze automaton; static proxy, not a rigged character.
cone('Cloak',(0,0,.65),.55,.27,1.3,'Slate',8)
ico('Shoulders',(0,0,1.2),(.5,.3,.25),'Bronze')
ico('Hood',(0,0,1.6),(.35,.32,.4),'Slate')
ico('Face',(-.28,0,1.58),(.08,.2,.19),'Amber')
beam('Staff',(-.12,.55,.1),(-.12,.55,2.05),.055,'Bronze')
ico('StaffCore',(-.12,.55,2.13),(.16,.16,.25),'Amber')
export('SM_Seeker')
pine(0,0,1);export('SM_Pine')
ico('Rock',(0,0,.35),(.9,.65,.65),'Stone');export('SM_Boulder')
# Landscape island, presentation only; no route or movement authority.
rng=random.Random(71029);N=24;verts=[]
for j in range(N+1):
 for i in range(N+1):
  x=-17+i*34/N;y=-17+j*34/N
  z=-.12 if abs(x)<13 and abs(y)<13 else -.25-((max(abs(x),abs(y))-13)*.24)
  if max(abs(x),abs(y))>12:z+=rng.uniform(-.12,.12)
  verts.append((x,y,z))
faces=[]
for j in range(N):
 for i in range(N):
  k=j*(N+1)+i;faces.extend([(k,k+1,k+N+2),(k,k+N+2,k+N+1)])
mesh=bpy.data.meshes.new('Island');mesh.from_pydata(verts,[],faces);mesh.update()
o=bpy.data.objects.new('Island',mesh);bpy.context.collection.objects.link(o)
finish(o,'Island','Grass');o.data.materials.append(MATS['Moss'])
for p in o.data.polygons:p.material_index=1 if rng.random()<.25 else 0
export('SM_Island')
# Put individual meshes in separate collections at original pivots for re-export.
for o in bpy.data.objects:o.hide_set(False)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'AmberMarch.blend'))
(OUT/'palette.json').write_text(json.dumps(PALETTE,indent=2)+'\n')
(OUT/'asset_manifest.json').write_text(json.dumps({'origin':'Original procedural art by this project; no third-party meshes','assets':stats},indent=2)+'\n')
print('AMBER_ART_PASS meshes='+str(len(stats))+' triangles='+str(sum(x['triangles'] for x in stats)))
