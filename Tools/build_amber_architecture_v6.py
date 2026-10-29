"""AIHEROES AmberMarch V6: original landmark silhouettes for 3D strategic map.

Blender 5.x command:
  blender --background --python Tools/build_amber_architecture_v6.py

Output is a new additive kit. Never overwrites existing project art.
No external meshes, textures, copied game designs, collision or navigation.
"""
import bpy
import math
import json
import hashlib
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "ArtSource" / "AmberMarch" / "ArchitectureV6"
if OUT.exists() and any(OUT.iterdir()):
    raise RuntimeError("ArchitectureV6 already exists; refusing to overwrite")
OUT.mkdir(parents=True, exist_ok=True)

bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system = "METRIC"

palette = json.loads((ROOT / "ArtSource" / "AmberMarch" / "palette.json").read_text())
M = {}
for key, color in palette.items():
    material = bpy.data.materials.new(key)
    material.diffuse_color = color
    material.use_nodes = True
    shader = material.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = color
    shader.inputs["Roughness"].default_value = .76 if key in ("Slate", "Stone") else .65
    M[key] = material

parts = []
receipt = []
completed = {}

def put(obj, name, color):
    obj.name = name
    obj.data.materials.append(M[color])
    parts.append(obj)
    return obj

def box(name, xyz, size, color):
    bpy.ops.mesh.primitive_cube_add(size=1, location=xyz)
    o = bpy.context.object
    o.scale = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return put(o, name, color)

def cylinder(name, xyz, radius, height, color, vertices=12):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius,
                                        depth=height, location=xyz)
    return put(bpy.context.object, name, color)

def spire(name, xyz, r1, r2, height, color, verts=8):
    bpy.ops.mesh.primitive_cone_add(vertices=verts, radius1=r1, radius2=r2,
                                    depth=height, location=xyz)
    return put(bpy.context.object, name, color)

def beam(name, a, b, thickness, color):
    av, bv = Vector(a), Vector(b)
    mid = (av + bv) / 2
    o = box(name, mid, (thickness, thickness, (bv-av).length), color)
    o.rotation_euler = (bv-av).to_track_quat("Z", "Y").to_euler()
    return o

def roof(name, x, y, bottom, width, depth, rise, color):
    # Simple 3D gabled roof; ridge points along world X.
    verts = [
        (x-width/2, y-depth/2, bottom),
        (x+width/2, y-depth/2, bottom),
        (x-width/2, y+depth/2, bottom),
        (x+width/2, y+depth/2, bottom),
        (x-width/2, y, bottom+rise),
        (x+width/2, y, bottom+rise),
    ]
    faces = [(0,1,5,4), (4,5,3,2), (0,4,2), (1,3,5)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return put(obj, name, color)

def banner(name, x, y, z, blue=True, east=False):
    beam(name+"Pole", (x,y,z), (x,y,z+1.24), .055, "Bronze")
    # Faceted hanging pennant with triangular notch, original symbol language.
    dx = .10 if east else -.10
    coords = [
        (x,y,z+1.13), (x+dx+(.56 if east else -.56),y,z+1.02),
        (x+dx+(.50 if east else -.50),y,z+.40),
        (x+dx+(.27 if east else -.27),y,z+.57),
        (x+dx,y,z+.40)
    ]
    mesh = bpy.data.meshes.new(name+"Fabric")
    mesh.from_pydata(coords,[],[(0,1,2,3,4)])
    mesh.update()
    obj = bpy.data.objects.new(name+"Fabric", mesh)
    bpy.context.collection.objects.link(obj)
    put(obj,name+"Fabric","Slate" if blue else "Bronze")
    box(name+"Sigil",(x+(.27 if east else -.27),y-.012,z+.80),
        (.10,.035,.22),"Amber" if blue else "Ivory")

def window(name, x, y, z, side="front"):
    if side == "front":
        box(name,(x,y,z),(.22,.035,.40),"Dark")
        box(name+"Sill",(x,y-.022,z-.23),(.34,.08,.07),"Bronze")
    else:
        box(name,(x,y,z),(.04,.21,.40),"Dark")
        box(name+"Sill",(x-.028,y,z-.23),(.08,.34,.07),"Bronze")

def export(name):
    global parts
    if not parts:
        raise RuntimeError("Empty model: "+name)
    bpy.ops.object.select_all(action="DESELECT")
    for o in parts:
        o.hide_set(False)
        o.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = name
    bpy.context.scene.cursor.location = (0,0,0)
    bpy.ops.object.origin_set(type="ORIGIN_CURSOR")
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    obj.data.calc_loop_triangles()
    tris = len(obj.data.loop_triangles)
    if not 100 < tris < 16000:
        raise RuntimeError("Geometry budget exceeded: %s %d" % (name,tris))
    # These meshes are decorative. The existing game's click collider remains authoritative.
    fbx = OUT / (name+".fbx")
    bpy.ops.export_scene.fbx(
        filepath=str(fbx), use_selection=True, object_types={"MESH"},
        apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
        axis_forward="-Y", axis_up="Z", bake_anim=False,
        add_leaf_bones=False, use_triangles=True)
    if fbx.stat().st_size < 2000:
        raise RuntimeError("Empty FBX: "+name)
    bbox = [list(v) for v in obj.bound_box]
    record = {
        "name": name, "tris": tris, "bytes": fbx.stat().st_size,
        "sha256": hashlib.sha256(fbx.read_bytes()).hexdigest(),
        "bounds_blender_m": {
            "min": [min(v[i] for v in bbox) for i in range(3)],
            "max": [max(v[i] for v in bbox) for i in range(3)]
        }
    }
    receipt.append(record)
    completed[name] = obj
    parts = []
    obj.hide_set(True)

# --------------------------------------------------------------------------
# CIT A D E L  — readable across the whole strategy map.
# 4 blue-capped corner towers, central keep, crest, gate, crenellations.
# --------------------------------------------------------------------------
box("CitadelPlinth",(0,0,.20),(6.5,5.8,.40),"Stone")
box("CitadelCourt",(0,0,.46),(5.9,5.2,.16),"Ivory")
for wall_y in (-2.58,2.58):
    box("OuterWall",(0,wall_y,1.56),(5.85,.34,2.22),"Ivory")
    for j in range(10):
        x=-2.58+j*.57
        if wall_y<0 and abs(x)<.77:
            continue
        box("Battlement",(x,wall_y,2.82),(.36,.40,.40),"Stone")
for wall_x in (-2.85,2.85):
    box("OuterWall",(wall_x,0,1.56),(.32,5.2,2.22),"Ivory")
    for j in range(8):
        y=-2.34+j*.67
        box("Battlement",(wall_x,y,2.83),(.38,.40,.34),"Stone")
for x in (-2.68,2.68):
    for y in (-2.37,2.37):
        cylinder("CornerBastion",(x,y,2.10),.79,4.00,"Stone",10)
        cylinder("CornerCrown",(x,y,4.13),.91,.32,"Ivory",10)
        spire("BlueTower",(x,y,4.88),1.03,0,1.53,"Slate",10)
        spire("Finial",(x,y,5.75),.12,0,.43,"Amber",8)
        for dx,dy in ((-.38,-.45),(.35,-.45)):
            box("TowerWindow",(x+dx,y+dy,2.60),(.16,.06,.42),"Dark")
        cylinder("TowerGoldBand",(x,y,3.20),.80,.11,"Bronze",10)

# Central noble keep and roofline.
box("KeepBody",(0,.42,2.70),(2.55,3.05,4.25),"Ivory")
box("KeepPlinth",(0,.42,.77),(2.84,3.34,.24),"Stone")
box("KeepUpperTrim",(0,.42,4.90),(2.86,3.36,.18),"Bronze")
roof("BlueKeepRoof",0,.42,5.03,3.05,3.61,1.40,"Slate")
box("KeepCrownTower",(0,.42,6.00),(.87,.87,1.10),"Stone")
spire("CrownRoof",(0,.42,6.90),.83,0,1.05,"Slate",8)
spire("SunSpire",(0,.42,7.50),.12,0,.36,"Amber",8)
for x in (-.72,.72):
    for z in (2.38,3.57):
        window("KeepLight",x,-1.13,z)
# Guardhouse gate, arch/door and gold sun above.
box("GateDoor",(0,-2.78,1.23),(1.33,.10,1.78),"Dark")
box("GateLintel",(0,-2.85,2.12),(1.72,.25,.26),"Stone")
for x in (-.80,.80):
    box("GatePillar",(x,-2.80,1.37),(.26,.40,1.9),"Stone")
box("GateSun",(0,-2.98,2.51),(.34,.12,.34),"Amber")
for x in (-1.68,1.68):
    window("OuterArchery",x,-2.78,1.43)
# Visible gold-blue royal banners, not a copyrighted coat of arms.
banner("WestBanner",-1.02,-1.20,5.10,True,False)
banner("EastBanner",1.04,-1.20,5.10,True,True)
# Castle courtyard paving, readable in angled camera.
for x in (-.82,0,.82):
    box("CourtTile",(x,-1.78,.58),(.62,.50,.07),"Path")
export("SM_CitadelCrownV6")

# --------------------------------------------------------------------------
# F R O N T I E R  G A T E — silhouette and guarded chokepoint visual only.
# --------------------------------------------------------------------------
box("FrontierFoundation",(0,0,.19),(5.20,3.25,.38),"Stone")
for x in (-1.79,1.79):
    cylinder("FrontierBastion",(x,0,1.93),.81,3.55,"Stone",10)
    cylinder("WatchParapet",(x,0,3.73),.95,.28,"Ivory",10)
    spire("WatchRoof",(x,0,4.52),1.01,0,1.34,"Slate",10)
    spire("WatchSpear",(x,0,5.23),.10,0,.35,"Amber",8)
    for z in (1.90,2.63):
        window("FrontierArrowSlit",x,-.78,z)
box("GateWall",(0,0,1.80),(3.0,1.30,3.0),"Ivory")
box("Portcullis",(0,-.69,1.35),(1.33,.13,2.18),"Dark")
for x in (-.38,0,.38):
    box("GateIronRib",(x,-.79,1.34),(.075,.10,2.22),"Bronze")
for z in (.66,1.30,1.95):
    box("GateIronCross",(0,-.79,z),(1.25,.11,.09),"Bronze")
box("GateTop",(0,0,3.42),(3.35,1.54,.24),"Stone")
for x in (-1.30,-.68,0,.68,1.30):
    box("GateCrenel",(x,0,3.70),(.38,1.54,.41),"Ivory")
banner("FrontierBanner",0,-.88,3.82,False,True)
export("SM_FrontierGateV6")

# --------------------------------------------------------------------------
# A R C H I V E  S A N C T U M — an original ruined ring + cyan core.
# --------------------------------------------------------------------------
cylinder("ArchivePlatform",(0,0,.24),2.37,.46,"Stone",12)
cylinder("ArchiveFloor",(0,0,.52),1.99,.12,"Ivory",12)
for idx in range(8):
    a = 2*math.pi*idx/8
    x,y = 1.58*math.cos(a),1.58*math.sin(a)
    height = 2.95 if idx%3 else 1.93
    cylinder("RuinedColumn",(x,y,.64+height/2),.24,height,"Stone",8)
    cylinder("ColumnBase",(x,y,.76),.38,.24,"Ivory",8)
    if idx%3:
        cylinder("ColumnCap",(x,y,.64+height),.35,.20,"Bronze",8)
for i in range(7):
    a = 2*math.pi*i/7
    x,y = 2.06*math.cos(a),2.06*math.sin(a)
    spire("BrokenShard",(x,y,.75),.32,.13,1.0+(.2*(i%3)),"Stone",6)
# Circular golden focus of the portal, radial crystal composition.
for idx in range(12):
    a=2*math.pi*idx/12
    x,y=.87*math.cos(a),.87*math.sin(a)
    cylinder("FocusTile",(x,y,.65),.14,.15,"Bronze",8)
cylinder("FocusBase",(0,0,.70),.64,.18,"Dark",12)
spire("EnergyPrism",(0,0,1.55),.45,.12,1.55,"Crystal",6)
spire("EnergyTop",(0,0,2.62),.24,0,.75,"Crystal",6)
# Fractured arch at back with a recognizable four-way frame.
for x in (-.85,.85):
    box("ArcPier",(x,1.5,2.15),(.47,.48,3.1),"Ivory")
box("ArcKeystone",(0,1.5,3.88),(2.20,.48,.45),"Ivory")
for x in (-.78,.78):
    box("ArcBronzeBand",(x,1.23,2.72),(.44,.08,.14),"Bronze")
export("SM_ArchiveSanctumV6")

# Keep a complete, author-editable Blender source file for iteration.
for obj in completed.values():
    obj.hide_set(False)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/"ArchitectureV6.blend"))

# Useful visually inspectable 16:9 preview of the FIRST ORIGINAL citadel.
for o in completed.values():
    o.hide_render = (o.name != "SM_CitadelCrownV6")
# Simple unobtrusive platform and studio illumination; never exported as FBX.
box("PreviewGround",(0,0,-.20),(10.3,9.3,.2),"Moss")
for obj in parts:
    obj.hide_render = False
bpy.ops.object.camera_add(location=(13,-17,13))
camera=bpy.context.object
direction=Vector((0,0,2.9))-camera.location
camera.rotation_euler=direction.to_track_quat("-Z","Y").to_euler()
camera.data.type="ORTHO"
camera.data.ortho_scale=15.4
bpy.context.scene.camera=camera
for location,power,size in [
    ((-7,-9,15),1600,9),((7,4,10),1150,8),((0,8,15),800,12)
]:
    bpy.ops.object.light_add(type="AREA",location=location)
    light=bpy.context.object
    light.data.energy=power
    light.data.shape="DISK"
    light.data.size=size
    light.rotation_euler=(Vector((0,0,2))-light.location).to_track_quat("-Z","Y").to_euler()
scene=bpy.context.scene
scene.render.engine="CYCLES"
scene.cycles.device="CPU"
scene.cycles.samples=12
scene.cycles.use_denoising=True
scene.render.resolution_x=800
scene.render.resolution_y=450
scene.render.resolution_percentage=100
scene.render.image_settings.file_format="PNG"
scene.render.filepath=str(OUT/"citadel_preview.png")
scene.world.color=(.055,.075,.10)
scene.view_settings.view_transform="AgX"
metadata={
    "title":"AIHEROES AmberMarch original architecture V6",
    "origin":"Entirely original procedural Blender geometry; user reference images are directional, not copied assets",
    "units":"Blender metres, FBX imported as centimetres",
    "collision":"visual-only; original game's actor collider remains authoritative",
    "assets":receipt,
}
(OUT/"asset_manifest.json").write_text(
    json.dumps(metadata,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
print("ARCHITECTURE_V6_BUILD_PASS assets=%d tris=%d" % (
    len(receipt),sum(item["tris"] for item in receipt)))
try:
    bpy.ops.render.render(write_still=True)
except Exception as exc:
    print("ARCHITECTURE_V6_PREVIEW_UNAVAILABLE "+str(exc))
