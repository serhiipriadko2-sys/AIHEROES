"""Independent Blender visual inspection of V5 source; NOT a game screenshot."""
import bpy, math, random
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
TERRAIN=ROOT/'ArtSource'/'AmberMarch'/'ReliefV5'/'ReliefV5.blend'
OUT=ROOT/'Saved'/'QA'/'ReliefV5_20261029'/'relief_preview.png'
OUT.parent.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(TERRAIN))
for obj in bpy.data.objects:
    obj.hide_set(False)
    obj.hide_render=obj.name!='SM_IslandReliefV5'
def append_object(filepath,name):
    with bpy.data.libraries.load(str(filepath),link=False) as (from_data,to_data):
        if name not in from_data.objects:
            raise RuntimeError('Missing source object '+name)
        to_data.objects=[name]
    item=to_data.objects[0]
    bpy.context.collection.objects.link(item)
    item.hide_set(False)
    item.hide_render=False
    return item
SOURCE=ROOT/'ArtSource'/'AmberMarch'/'AmberMarch.blend'
for name,location in (
    ('SM_Hearth',(-6.5,0,0)),('SM_Sawmill',(-1,-6.5,0)),
    ('SM_Quarry',(1.5,6,0)),('SM_Watchtower',(7,-3,0)),
    ('SM_Archive',(7.5,6.5,0))
):
    o=append_object(SOURCE,name)
    o.location=location
waterblend=ROOT/'ArtSource'/'AmberMarch'/'RiverV4'/'RiverV4.blend'
angle=math.atan2(5.5,6.5)
for name in ('SM_Silverbrook','SM_BrookBridge'):
    item=append_object(waterblend,name)
    item.location=(-3.75,-3.25,0)
    item.rotation_euler.z=angle
# Retain 40 nonblocking authored pines near the central fields
pine=append_object(SOURCE,'SM_Pine')
pine.hide_render=True
randomizer=random.Random(3913)
site_xy=[(-6.5,0),(-1,-6.5),(1.5,6),(7,-3),(7.5,6.5)]
for i in range(68):
    x=randomizer.uniform(-12,12);y=randomizer.uniform(-12,12)
    if any(math.hypot(x-a,y-b)<2.7 for a,b in site_xy):
        continue
    if -7<x<4 and -3<y<3:
        continue
    copy=pine.copy();copy.data=pine.data
    bpy.context.collection.objects.link(copy)
    copy.location=(x,y,-.12)
    k=randomizer.uniform(.65,1.04)
    copy.scale=(k,k,k)
    copy.hide_render=False
scene=bpy.context.scene
scene.render.engine='BLENDER_WORKBENCH'
scene.display.shading.color_type='MATERIAL'
scene.display.shading.light='STUDIO'
scene.display.shading.show_shadows=True
scene.display.shading.show_cavity=True
scene.render.resolution_x=1680;scene.render.resolution_y=1050
scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
scene.render.filepath=str(OUT)
bpy.ops.object.camera_add(location=(-27,-36,31))
cam=bpy.context.object
target=Vector((0,0,-.15))
cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler()
cam.data.type='ORTHO';cam.data.ortho_scale=43
scene.camera=cam
scene.view_settings.view_transform='Standard'
bpy.ops.render.render(write_still=True)
print('RELIEF_V5_BLENDER_PREVIEW_PASS bytes='+str(OUT.stat().st_size))