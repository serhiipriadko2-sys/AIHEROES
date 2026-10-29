"""Review render from the actual Blender meshes; not a screenshot of the Unreal game."""
import bpy, math
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[1]
src=root/'ArtSource'/'AmberMarch'/'SceneryV3'/'SceneryV3.blend'
out=root/'Saved'/'QA'/'SceneryV3_20261029'/'scenery_preview.png'
out.parent.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(src))
items={'SM_RidgeOutcrop':(-8,0,0),'SM_Waystone':(0,0,0),'SM_Waystation':(8,0,0)}
for o in bpy.data.objects:
    if o.type=='MESH' and o.name in items:
        o.hide_set(False)
        o.location=items[o.name]
        o.hide_render=False
    else:o.hide_render=True
scene=bpy.context.scene
scene.render.engine='BLENDER_WORKBENCH'
scene.display.shading.color_type='MATERIAL'
scene.display.shading.light='STUDIO'
scene.display.shading.show_shadows=True
scene.display.shading.show_cavity=True
scene.render.resolution_x=1536
scene.render.resolution_y=768
scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
scene.render.filepath=str(out)
bpy.ops.object.camera_add(location=(12,-26,18))
camera=bpy.context.object
target=Vector((0,0,1.0))
camera.rotation_euler=(target-camera.location).to_track_quat('-Z','Y').to_euler()
camera.data.type='ORTHO'
camera.data.ortho_scale=27.5
scene.camera=camera
scene.view_settings.view_transform='Standard'
bpy.ops.render.render(write_still=True)
print('SCENERY_V3_PREVIEW_PASS bytes='+str(out.stat().st_size))