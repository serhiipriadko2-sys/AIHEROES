"""Read-only audit of imported river material face normals in original Blender source."""
import bpy, json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=root/'ArtSource'/'AmberMarch'/'RiverV4'/'RiverV4.blend'
bpy.ops.wm.open_mainfile(filepath=str(source))
obj=bpy.data.objects.get('SM_Silverbrook')
if obj is None:raise RuntimeError('River asset missing')
data={}
for polygon in obj.data.polygons:
    name=obj.material_slots[polygon.material_index].material.name
    item=data.setdefault(name,{'up':0,'down':0,'other':0})
    category='down' if polygon.normal.z<-.8 else 'up' if polygon.normal.z>.8 else 'other'
    item[category]+=1
print('RIVER_NORMALS_AUDIT '+json.dumps(data,sort_keys=True))