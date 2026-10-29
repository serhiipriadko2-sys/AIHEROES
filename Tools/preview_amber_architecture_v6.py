"""Low-memory CPU previews of the two original AmberMarch V6 landmarks.

Run with the authoritative ArchitectureV6.blend already loaded; no existing
FBX, .blend, imported .uasset or first preview is modified.
"""
import bpy
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/"ArtSource"/"AmberMarch"/"ArchitectureV6"
TARGETS=(
    ("SM_FrontierGateV6","frontier_preview.png",11.5,2.2),
    ("SM_ArchiveSanctumV6","archive_preview.png",10.5,1.9)
)
for name, filename, _, _ in TARGETS:
    if bpy.data.objects.get(name) is None:
        raise RuntimeError("Missing original model "+name)
    if (OUT/filename).exists():
        raise RuntimeError("Refusing existing preview "+filename)
scene=bpy.context.scene
scene.render.engine="CYCLES"
scene.cycles.device="CPU"
scene.cycles.samples=8
scene.cycles.use_denoising=True
scene.render.resolution_x=800
scene.render.resolution_y=450
scene.render.resolution_percentage=100
scene.render.image_settings.file_format="PNG"
scene.world.color=(.056,.079,.11)
scene.view_settings.view_transform="AgX"
mat=bpy.data.materials.get("Moss")
if mat is None:
    mat=bpy.data.materials.new("Moss")
    mat.diffuse_color=(.25,.34,.22,1)
    mat.use_nodes=True
    mat.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value=mat.diffuse_color
bpy.ops.mesh.primitive_cube_add(size=1,location=(0,0,-.21))
ground=bpy.context.object
ground.name="V6PreviewGroundOnly"
ground.scale=(9.3,8.4,.2)
ground.data.materials.append(mat)
bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
bpy.ops.object.camera_add(location=(11,-15,12))
camera=bpy.context.object
camera.name="V6PreviewCamera"
camera.data.type="ORTHO"
scene.camera=camera
for xyz,power,size in (
    ((-7,-8,13),1450,9),
    ((6,5,11),1080,10),
    ((1,8,15),750,11)
):
    bpy.ops.object.light_add(type="AREA",location=xyz)
    lamp=bpy.context.object
    lamp.data.energy=power
    lamp.data.shape="DISK"
    lamp.data.size=size
    lamp.rotation_euler=(Vector((0,0,1))-lamp.location).to_track_quat("-Z","Y").to_euler()
for name,filename,size,target_z in TARGETS:
    for object_name in ("SM_CitadelCrownV6","SM_FrontierGateV6","SM_ArchiveSanctumV6"):
        bpy.data.objects[object_name].hide_render=object_name!=name
    camera.data.ortho_scale=size
    camera.rotation_euler=(Vector((0,0,target_z))-camera.location).to_track_quat("-Z","Y").to_euler()
    scene.render.filepath=str(OUT/filename)
    bpy.ops.render.render(write_still=True)
    if not (OUT/filename).exists() or (OUT/filename).stat().st_size<4000:
        raise RuntimeError("Preview render failed "+filename)
    print("ARCHITECTURE_V6_PREVIEW_PASS "+filename+" bytes="+str((OUT/filename).stat().st_size))
