"""AIHEROES Relief V5: genuine low-poly heightfield + recessed river channel.
Original Blender geometry. New assets only; never overwrite the V1 island.
Two identical terrain profiles at desktop/mobile vertex densities.
"""
import bpy, math, json, hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'ArtSource'/'AmberMarch'/'ReliefV5'
if OUT.exists() and any(OUT.iterdir()):
    raise RuntimeError('ReliefV5 already exists. Refuse overwrite.')
OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
palette=json.loads((ROOT/'ArtSource'/'AmberMarch'/'palette.json').read_text(encoding='utf-8'))
materials={}
for key in ('Grass','Moss','Stone','Path'):
    color=palette[key]
    m=bpy.data.materials.new(key)
    m.diffuse_color=color
    m.use_nodes=True
    node=m.node_tree.nodes.get('Principled BSDF')
    node.inputs['Base Color'].default_value=color
    node.inputs['Roughness'].default_value=.9
    materials[key]=m
SITES=[(-6.5,0),(-1,-6.5),(1.5,6),(7,-3),(7.5,6.5)]
LINKS=((0,1),(0,2),(1,3),(2,4),(3,4))
HILLS=(
    (-11,8,1.3,2.8,3.5),(-10,-10,1.55,3.2,3.4),
    (-2.5,12.6,1.35,3.6,2.5),(10.8,11.0,1.6,3.3,3.0),
    (12.2,-8,1.4,2.9,3.5),(3.7,-12.6,1.25,3.5,2.3),
    (-13,1.5,1.0,2.3,4.2),(12.7,0,1.15,2.7,3.5),
)
SQRT=lambda x:math.sqrt(x)
def smooth(a,b,v):
    t=max(0.0,min(1.0,(v-a)/(b-a)))
    return t*t*(3-2*t)
def segment_distance(x,y,ax,ay,bx,by):
    vx,vy=bx-ax,by-ay
    den=vx*vx+vy*vy
    t=max(0.,min(1.,((x-ax)*vx+(y-ay)*vy)/den))
    return math.hypot(x-(ax+t*vx),y-(ay+t*vy))
def height(x,y):
    e=max(abs(x),abs(y))
    if e>13:
        return -.25-(e-13)*.24 + .027*math.sin(x*2.37+y*1.44)
    site=min(math.hypot(x-a,y-b) for a,b in SITES)
    rd=min(segment_distance(x,y,*SITES[i],*SITES[j]) for i,j in LINKS)
    mask=smooth(2.35,4.5,site)*(.12+.88*smooth(1.25,3.8,rd))
    hills=sum(a*math.exp(-((x-cx)/sx)**2-((y-cy)/sy)**2)
        for cx,cy,a,sx,sy in HILLS)
    fine=.07*math.sin(.98*x+1.17*y)*math.cos(1.29*x-.64*y)
    baseline=-.12+(hills+fine)*mask
    # Exact local coordinate frame of the HQ(-650,0)->FOREST(-100,-650)
    # scenic crossing, converted to metres.
    midx,midy=-3.75,-3.25
    angle=math.atan2(5.5,6.5)
    ca,sa=math.cos(angle),math.sin(angle)
    lx=(x-midx)*ca+(y-midy)*sa
    ly=-(x-midx)*sa+(y-midy)*ca
    if abs(lx)<5.6:
        taper=smooth(0,.7,5.6-abs(lx))
        yc=.13*math.sin(lx*.74)+.08*math.cos(1.47*lx)
        half=.64+.07*math.sin(.67*lx+.4)
        dist=abs(ly-yc)
        if dist<=half-.28:
            river=-.50
        elif dist<=half:
            river=-.50+.625*smooth(half-.28,half,dist)
        elif dist<=half+.27:
            river=.125+.01*smooth(half,half+.27,dist)
        elif dist<=half+1.12:
            river=.135+(baseline-.135)*smooth(half+.27,half+1.12,dist)
        else:
            river=baseline
        baseline+=(river-baseline)*taper
    return baseline
def terrain(name,n):
    step=34/n
    vertices=[]
    for j in range(n+1):
        y=-17+j*step
        for i in range(n+1):
            x=-17+i*step
            vertices.append((x,y,height(x,y)))
    faces=[]
    for j in range(n):
        for i in range(n):
            k=j*(n+1)+i
            faces.extend([(k,k+1,k+n+2),(k,k+n+2,k+n+1)])
    m=bpy.data.meshes.new(name)
    m.from_pydata(vertices,[],faces)
    m.update()
    o=bpy.data.objects.new(name,m)
    bpy.context.collection.objects.link(o)
    for key in ('Grass','Moss','Stone','Path'):
        m.materials.append(materials[key])
    for poly in m.polygons:
        c=poly.center
        if max(abs(c.x),abs(c.y))>12.7:
            idx=2  # stone cliffs at shoreline
        elif poly.normal.z<.77 or c.z>.67:
            idx=2  # exposed upper rock and steep slopes
        elif c.z<-.28:
            idx=3  # river floor
        elif math.sin(c.x*3.3+c.y*1.2)*math.cos(c.y*2.7)<-.27:
            idx=1  # mossy patches
        else:
            idx=0
        poly.material_index=idx
    o.data.calc_loop_triangles()
    triangles=len(o.data.loop_triangles)
    if triangles != n*n*2:
        raise RuntimeError('Unexpected triangle count')
    bpy.ops.object.select_all(action='DESELECT')
    o.select_set(True)
    bpy.context.view_layer.objects.active=o
    file=OUT/(name+'.fbx')
    bpy.ops.export_scene.fbx(filepath=str(file),use_selection=True,
        object_types={'MESH'},apply_unit_scale=True,
        apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',
        bake_anim=False,add_leaf_bones=False,use_triangles=True)
    if file.stat().st_size<20000:
        raise RuntimeError('Invalid file '+name)
    return dict(name=name,grid=n,triangles=triangles,bytes=file.stat().st_size,
        sha256=hashlib.sha256(file.read_bytes()).hexdigest())
assert all(abs(height(*site)+.12)<1e-6 for site in SITES), 'Strategic pads must be level'
assert height(-3.75,-3.25)<-.40, 'Riverbed must be deeper than water surface'
assert height(-11,8)>.24, 'Relief hills must actually rise'
assets=[terrain('SM_IslandReliefV5',120),terrain('SM_IslandReliefV5_Mobile',72)]
samples=[dict(point_cm=[int(round(x*100)),int(round(y*100))],
    height_cm=round(height(x,y)*100,5))
    for x,y in SITES+[(-3.75,-3.25),(-11,8),(-10,-10),(12,0)]]
manifest={'origin':'Original procedural topology; no third-party art',
    'art_scope':'Visual terrain only; no collision, physics or path changes',
    'profile':'One height function, separate desktop and Android mesh density',
    'assets':assets,'strategic_site_samples':samples}
for obj in bpy.data.objects:obj.hide_set(False)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'ReliefV5.blend'))
(OUT/'asset_manifest.json').write_text(
    json.dumps(manifest,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
print('RELIEF_V5_BUILD_PASS assets=2 triangles='+str(sum(a['triangles'] for a in assets)))
