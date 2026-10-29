"""One-time recovery of a partially exported V6 authoring run.

Run AFTER loading ArchitectureV6.blend. Reads and hashes already-created FBXs
without regenerating or overwriting them. Fails closed on any unexpected source.
"""
import bpy
import json
import hashlib
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/"ArtSource"/"AmberMarch"/"ArchitectureV6"
MANIFEST=OUT/"asset_manifest.json"
PREVIEW=OUT/"citadel_preview.png"
if MANIFEST.exists() or PREVIEW.exists():
    raise RuntimeError("Refuse to overwrite existing V6 receipt or render")
models=("SM_CitadelCrownV6","SM_FrontierGateV6","SM_ArchiveSanctumV6")
records=[]
for name in models:
    obj=bpy.data.objects.get(name)
    fbx=OUT/(name+".fbx")
    if obj is None or obj.type!="MESH" or not fbx.is_file():
        raise RuntimeError("Expected original mesh missing: "+name)
    if fbx.stat().st_size<2000:
        raise RuntimeError("Empty asset "+name)
    obj.data.calc_loop_triangles()
    tris=len(obj.data.loop_triangles)
    if not 100<tris<16000:
        raise RuntimeError("Triangle budget invalid "+name)
    bbox=[list(v) for v in obj.bound_box]
    records.append({
        "name":name,"tris":tris,"bytes":fbx.stat().st_size,
        "sha256":hashlib.sha256(fbx.read_bytes()).hexdigest(),
        "bounds_blender_m":{
            "min":[min(v[i] for v in bbox) for i in range(3)],
            "max":[max(v[i] for v in bbox) for i in range(3)]
        }
    })
    obj.hide_render=name!="SM_CitadelCrownV6"
palette=json.loads((ROOT/"ArtSource"/"AmberMarch"/"palette.json").read_text())
material=bpy.data.materials.get("Moss")
if not material:
    material=bpy.data.materials.new("Moss")
    material.diffuse_color=palette["Moss"]
    material.use_nodes=True
    material.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value=palette["Moss"]
bpy.ops.mesh.primitive_cube_add(size=1,location=(0,0,-.2))
ground=bpy.context.object
ground.name="CitadelPreviewOnlyGround"
ground.scale=(10.3,9.3,.2)
ground.data.materials.append(material)
bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
bpy.ops.object.camera_add(location=(13,-17,13))
camera=bpy.context.object
camera.name="CitadelPreviewCamera"
camera.rotation_euler=(Vector((0,0,2.9))-camera.location).to_track_quat("-Z","Y").to_euler()
camera.data.type="ORTHO"
camera.data.ortho_scale=15.4
bpy.context.scene.camera=camera
for xyz,energy,size in (
    ((-7,-9,15),1600,9),((7,4,10),1150,8),((0,8,15),800,12)
):
    bpy.ops.object.light_add(type="AREA",location=xyz)
    light=bpy.context.object
    light.data.energy=energy
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
scene.render.filepath=str(PREVIEW)
scene.world.color=(.055,.075,.10)
scene.view_settings.view_transform="AgX"
metadata={
    "title":"AIHEROES AmberMarch original architecture V6",
    "origin":"Original procedural Blender geometry; no source-game meshes or reference-image pixels copied",
    "collision":"Visual only, original actor colliders authoritative",
    "assets":records
}
MANIFEST.write_text(json.dumps(metadata,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
print("ARCHITECTURE_V6_ASSETS_VERIFIED assets=%d total_tris=%d" %
    (len(records),sum(x["tris"] for x in records)))
try:
    bpy.ops.render.render(write_still=True)
except Exception as err:
    print("ARCHITECTURE_V6_PREVIEW_FAILED "+str(err))
print("ARCHITECTURE_V6_PREVIEW_STATUS="+
    ("PASS" if PREVIEW.exists() and PREVIEW.stat().st_size>4000 else "PENDING"))
