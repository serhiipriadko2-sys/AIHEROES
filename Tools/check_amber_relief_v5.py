"""Independent Relief V5 geometry audit (Blender 5.2).
READ ONLY: does not alter project meshes, materials or saves.
"""
import bpy,math,json,hashlib
from pathlib import Path
root=Path(__file__).resolve().parents[1]
folder=root/'ArtSource'/'AmberMarch'/'ReliefV5'
qa=root/'Saved'/'QA'/'ReliefV5_20261029'
manifest=json.loads((folder/'asset_manifest.json').read_text(encoding='utf-8'))
bpy.ops.wm.open_mainfile(filepath=str(folder/'ReliefV5.blend'))
meshstats=[]
samples={}
for spec in manifest['assets']:
    name=spec['name']
    obj=bpy.data.objects.get(name)
    if not obj or obj.type!='MESH':
        raise RuntimeError('Missing mesh '+name)
    mesh=obj.data
    n=spec['grid']
    if len(mesh.vertices)!=(n+1)*(n+1):
        raise RuntimeError('Wrong vertex count '+name)
    mesh.calc_loop_triangles()
    if len(mesh.loop_triangles)!=2*n*n or spec['triangles']!=2*n*n:
        raise RuntimeError('Wrong triangle count '+name)
    if any(not all(math.isfinite(v) for v in vertex.co) for vertex in mesh.vertices):
        raise RuntimeError('Invalid point '+name)
    if any(face.area<=1e-9 for face in mesh.polygons):
        raise RuntimeError('Degenerate polygon '+name)
    materials={slot.name for slot in mesh.materials}
    if not {'Grass','Moss','Stone','Path'}.issubset(materials):
        raise RuntimeError('Missing material '+name)
    fbx=folder/(name+'.fbx')
    if hashlib.sha256(fbx.read_bytes()).hexdigest()!=spec['sha256']:
        raise RuntimeError('FBX drift '+name)
    if fbx.stat().st_size!=spec['bytes']:
        raise RuntimeError('FBX byte size drift '+name)
    heights=[v.co.z for v in mesh.vertices]
    if max(heights)<1.0 or min(heights)>-1.1:
        raise RuntimeError('Not a genuine relief '+name)
    # Selected vertex positions should be close to five original site coordinates
    for rec in manifest['strategic_site_samples'][:5]:
        x,y=[value/100 for value in rec['point_cm']]
        v=min(mesh.vertices,key=lambda vert:(vert.co.x-x)**2+(vert.co.y-y)**2)
        if math.hypot(v.co.x-x,v.co.y-y)>.30 or abs(v.co.z+.12)>.025:
            raise RuntimeError('Node pad not safely flat '+name+':'+str((x,y,v.co.z)))
    mid=min(mesh.vertices,
        key=lambda vert:(vert.co.x+3.75)**2+(vert.co.y+3.25)**2)
    if mid.co.z>-.37:
        raise RuntimeError('Channel is not carved under Silverbrook '+name)
    samples[name]={ (round(v.co.x,6),round(v.co.y,6)):round(v.co.z,7)
        for v in mesh.vertices }
    meshstats.append(dict(name=name,vertices=len(mesh.vertices),
        triangles=len(mesh.loop_triangles),
        height_min_m=round(min(heights),4),height_max_m=round(max(heights),4),
        materials=sorted(materials),source_sha256=spec['sha256']))
# Cross-resolution identity at matching grid sample sites
desktop=samples['SM_IslandReliefV5']
mobile=samples['SM_IslandReliefV5_Mobile']
matching=0
for xy,z in mobile.items():
    if xy in desktop:
        matching+=1
        if abs(desktop[xy]-z)>.000005:
            raise RuntimeError('Different height profiles for PC/mobile at '+str(xy))
if matching!=625:
    raise RuntimeError('Unexpected cross-resolution sample overlap '+str(matching))
report={'status':'PASS_ART_GEOMETRY_ONLY','assets':meshstats,
    'shared_vertex_height_checks':matching,
    'imported_into_unreal':(qa.parent/'relief_v5_import_receipt.json').exists(),
    'unreal_gameplay_tests_passed':False,
    'android_device_tested':False}
out=qa/'mesh_qc.json'
out.write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
assert json.loads(out.read_text(encoding='utf-8'))['status']=='PASS_ART_GEOMETRY_ONLY'
print('RELIEF_V5_MESH_QC_PASS triangles='+str(sum(x['triangles'] for x in meshstats))+
      ' shared='+str(matching))