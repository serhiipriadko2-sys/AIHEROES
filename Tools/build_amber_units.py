"""Original six-unit Amber Vanguard kit. Blender-only, no third-party assets.
Creates ArtSource/AmberVanguard once. Does not modify existing AmberMarch art.
"""
import bpy, math, json, hashlib
from pathlib import Path
from mathutils import Vector
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'ArtSource' / 'AmberVanguard'
if OUT.exists():
    raise RuntimeError('Refuse to overwrite existing unit art; inspect and version it first')
OUT.mkdir(parents=True)
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
COLORS = {'Cloth':(.045,.27,.29,1), 'Ivory':(.64,.69,.60,1),
    'Bronze':(.48,.25,.075,1), 'Amber':(1,.57,.12,1), 'Dark':(.035,.055,.06,1),
    'Leather':(.28,.12,.055,1), 'Rust':(.54,.15,.065,1),
    'Fur':(.40,.45,.44,1), 'PaleFur':(.66,.68,.60,1), 'Stone':(.33,.39,.35,1)}
MATS = {}
for name, color in COLORS.items():
    m = bpy.data.materials.new(name); m.diffuse_color = color
    MATS[name] = m
parts = []; records = []; models = []
def finish(o, name, mat):
    o.name = name; o.data.materials.append(MATS[mat]); parts.append(o); return o
def box(name, at, size, mat, rot=(0,0,0)):
    bpy.ops.mesh.primitive_cube_add(size=1, location=at)
    o=bpy.context.object; o.scale=size; o.rotation_euler=rot
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return finish(o,name,mat)
def cone(name,at,r1,r2,h,mat,n=8):
    bpy.ops.mesh.primitive_cone_add(vertices=n,radius1=r1,radius2=r2,depth=h,location=at)
    return finish(bpy.context.object,name,mat)
def ico(name,at,size,mat,sub=1):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=sub,radius=1,location=at)
    o=bpy.context.object; o.scale=size
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    return finish(o,name,mat)
def rod(name,a,b,width,mat):
    a,b=Vector(a),Vector(b); o=box(name,(a+b)*.5,(width,width,(b-a).length),mat)
    o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler(); return o
def person(cloth='Cloth', heavy=False):
    for y in [-.16,.16]:
        box('Boot',(.09,y,.12),(.36,.23,.24),'Leather')
        rod('Leg',(0,y,.23),(-.04,y,.72),.18,'Dark')
    cone('Tabard',(-.03,0,.73),.35,.24,.42,cloth,6)
    ico('Torso',(0,0,1.03),(.27,.33,.36),'Ivory' if heavy else cloth,1)
    box('Belt',(.015,0,.82),(.42,.56,.12),'Bronze')
    ico('Head',(.01,0,1.43),(.19,.20,.23),'Ivory',1)
    box('Eyes',(.177,0,1.46),(.035,.25,.055),'Dark')
    for y in [-.34,.34]:
        ico('Shoulder',(0,y,1.2),(.20,.17,.16),'Bronze' if heavy else cloth)
        rod('UpperArm',(0,y,1.18),(.10,y*1.12,.92),.14,cloth)
        ico('Hand',(.18,y*1.13,.90),(.10,.09,.09),'Ivory')
def shield():
    o=cone('Shield',(.12,-.48,.93),.36,.36,.08,'Bronze',6)
    o.rotation_euler.x=math.pi/2
    o=cone('ShieldFace',(.12,-.53,.93),.28,.28,.018,'Cloth',6)
    o.rotation_euler.x=math.pi/2
    ico('ShieldBoss',(.12,-.56,.93),(.085,.05,.085),'Amber')
def helm():
    cone('Helm',(0,0,1.55),.22,.18,.25,'Bronze',8)
    box('Visor',(.19,0,1.49),(.075,.30,.07),'Dark')
    box('NoseGuard',(.24,0,1.44),(.035,.035,.21),'Ivory')
# Portraits use the exact exported mesh, not a concept rendering.
bpy.ops.object.camera_add(location=(3.2,-5.5,2.8)); camera=bpy.context.object
camera.rotation_euler=(Vector((0,0,.95))-camera.location).to_track_quat('-Z','Y').to_euler()
camera.data.type='ORTHO'; camera.data.ortho_scale=2.35; scene.camera=camera
scene.render.engine='BLENDER_WORKBENCH'
scene.display.shading.light='STUDIO'; scene.display.shading.color_type='MATERIAL'
scene.display.shading.show_shadows=True; scene.display.shading.show_cavity=True
scene.display.shading.cavity_type='BOTH'; scene.display.shading.show_specular_highlight=True
scene.render.resolution_x=256; scene.render.resolution_y=256; scene.render.resolution_percentage=100
scene.render.film_transparent=True; scene.render.image_settings.file_format='PNG'
scene.render.image_settings.color_mode='RGBA'
scene.view_settings.view_transform='Standard'
def export(unit):
    global parts
    bpy.ops.object.select_all(action='DESELECT')
    for p in parts:p.select_set(True)
    bpy.context.view_layer.objects.active=parts[0]; bpy.ops.object.join()
    o=bpy.context.object; o.name='SM_'+unit
    scene.cursor.location=(0,0,0); bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    o.data.calc_loop_triangles(); tris=len(o.data.loop_triangles)
    assert 50 <= tris <= 1800, (unit,tris)
    f=OUT/(o.name+'.fbx')
    bpy.ops.export_scene.fbx(filepath=str(f),use_selection=True,object_types={'MESH'},
        apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',
        bake_anim=False,add_leaf_bones=False,use_triangles=True)
    for previous in models:previous.hide_render=True
    o.hide_render=False; scene.render.filepath=str(OUT/('T_'+unit+'.png'))
    bpy.ops.render.render(write_still=True)
    portrait=OUT/('T_'+unit+'.png')
    records.append({'unit':unit,'mesh':o.name,'triangles':tris,'material_slots':[m.name for m in o.data.materials],
        'fbx_bytes':f.stat().st_size,'fbx_sha256':hashlib.sha256(f.read_bytes()).hexdigest(),
        'portrait_bytes':portrait.stat().st_size,'portrait_sha256':hashlib.sha256(portrait.read_bytes()).hexdigest()})
    models.append(o); parts=[]; o.hide_set(True); o.hide_render=True
# Militia: a tall spear and a broad hexagonal shield.
person(heavy=True); helm(); shield()
rod('Spear',(.24,.43,.08),(.24,.43,1.86),.045,'Leather')
cone('SpearTip',(.24,.43,1.97),.085,0,.23,'Ivory',4)
export('Militia')
# Scout: green hood, compact bow, quiver. Existing rules already give range 3.
person(); ico('Hood',(-.04,0,1.50),(.24,.25,.27),'Cloth')
box('Face',(.20,0,1.47),(.06,.22,.20),'Ivory')
box('ScoutEyes',(.235,0,1.51),(.02,.18,.035),'Dark')
rod('Quiver',(-.24,.15,.72),(-.31,.15,1.35),.17,'Leather')
for y in [.10,.16,.22]:rod('Arrow',(-.31,y,1.15),(-.35,y,1.56),.025,'Ivory')
bow=[(.36,-.40,.55),(.52,-.40,.76),(.57,-.40,1.05),(.50,-.40,1.34),(.34,-.40,1.53)]
for a,b in zip(bow,bow[1:]):rod('Bow',a,b,.045,'Bronze')
rod('BowString',bow[0],bow[-1],.012,'Ivory')
export('Scout')
# Wolf: low quadruped with long muzzle, ears and tail.
ico('WolfBody',(0,0,.57),(.57,.24,.30),'Fur',2)
ico('Ruff',(.32,0,.72),(.32,.30,.38),'PaleFur')
ico('Skull',(.53,0,.92),(.24,.21,.23),'Fur')
ico('Muzzle',(.76,0,.85),(.29,.13,.12),'PaleFur')
ico('Nose',(.97,0,.87),(.075,.10,.07),'Dark')
for y in [-.14,.14]:
    cone('Ear',(.48,y,1.16),.12,0,.30,'Fur',4)
    ico('Eye',(.67,y*1.35,.98),(.04,.025,.025),'Amber')
for x in [-.32,.33]:
 for y in [-.19,.19]:
    rod('WolfLeg',(x,y,.52),(x+.04,y,.16),.10,'Fur')
    box('Paw',(x+.10,y,.08),(.22,.13,.14),'PaleFur')
rod('Tail',(-.46,0,.64),(-.88,.02,.90),.14,'Fur')
export('Wolf')
# Bandit: leather mask, short sword and stolen bronze plates.
person('Leather');ico('Mask',(.14,0,1.43),(.10,.22,.15),'Rust')
box('BanditEyes',(.23,0,1.49),(.02,.22,.04),'Dark')
rod('SwordGrip',(.18,.39,.85),(.35,.40,1.00),.07,'Leather')
rod('SwordBlade',(.35,.40,1.00),(.72,.42,1.48),.08,'Ivory')
box('Guard',(.33,.40,.99),(.08,.28,.06),'Bronze')
ico('Pack',(-.25,0,1.03),(.18,.32,.30),'Dark')
export('Bandit')
# Raider: hornless iron helmet, asymmetrical axe and red mantle.
person('Rust',heavy=True);helm()
box('Mantle',(-.24,0,1.03),(.08,.65,.72),'Rust')
rod('AxeShaft',(.18,.42,.45),(.20,.42,1.68),.065,'Leather')
box('AxeHead',(.23,.42,1.55),(.40,.11,.29),'Ivory',rot=(0,.12,0))
box('AxeEdge',(.46,.42,1.55),(.08,.14,.32),'Bronze')
export('Raider')
# Guardian: massive stone automaton, amber core, unmistakable broad shoulders.
for y in [-.26,.26]:
    box('StoneFoot',(.12,y,.13),(.48,.34,.26),'Stone')
    box('StoneLeg',(0,y,.47),(.30,.27,.50),'Ivory')
ico('GolemTorso',(0,0,1.04),(.46,.46,.50),'Stone')
box('CoreFrame',(.37,0,1.15),(.13,.30,.32),'Bronze')
ico('CognitionCore',(.45,0,1.15),(.10,.17,.18),'Amber')
for y in [-.55,.55]:
    ico('Pauldron',(0,y,1.39),(.34,.32,.29),'Ivory')
    box('Forearm',(.08,y,.95),(.26,.29,.47),'Stone')
    ico('Fist',(.18,y,.69),(.22,.22,.20),'Bronze')
box('GolemHead',(0,0,1.61),(.43,.42,.34),'Ivory')
box('EyeSlit',(.225,0,1.65),(.025,.29,.055),'Amber')
export('Guardian')
# Editable collection: one visible unit initially, all six retained at their ground pivot.
for i,o in enumerate(models):o.hide_set(i!=0);o.hide_render=i!=0
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'AmberVanguard.blend'))
(OUT/'palette.json').write_text(json.dumps(COLORS,indent=2)+'\n',encoding='utf-8')
(OUT/'asset_manifest.json').write_text(json.dumps({'origin':'Original procedural meshes and same-mesh portraits; no external assets',
    'forward':'+X','units':'metres, FBX unit conversion to Unreal centimetres','assets':records},indent=2)+'\n',encoding='utf-8')
print('AMBER_UNITS_PASS meshes='+str(len(records))+' portraits='+str(len(records))+' triangles='+str(sum(r['triangles'] for r in records)))
