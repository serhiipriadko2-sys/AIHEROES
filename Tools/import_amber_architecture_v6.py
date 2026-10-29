"""Editor-only import of three entirely original AmberMarch V6 architectural meshes.

Usage on an IDLE Unreal 5.8 editor command process:
  UnrealEditor-Cmd.exe AIHeroes.uproject -ExecutePythonScript=<this script> -unattended

Fail-closed rules:
- source FBX hashes MUST match the authoring manifest;
- refuse pre-existing Unreal asset paths (no asset replacement or migration);
- import only 3 static meshes, no collision or material generation;
- bind existing project's safe original palette material instances;
- verify imports and save in /Game/AmberMarch/Meshes.
"""
import unreal
import pathlib
import hashlib
import json

ROOT=pathlib.Path(unreal.Paths.project_dir()).resolve()
SRC=ROOT/"ArtSource"/"AmberMarch"/"ArchitectureV6"
SPEC=SRC/"asset_manifest.json"
EXPECTED=("SM_CitadelCrownV6","SM_FrontierGateV6","SM_ArchiveSanctumV6")
if not SPEC.is_file():
    raise RuntimeError("ArchitectureV6 authoring receipt missing")
manifest=json.loads(SPEC.read_text(encoding="utf-8"))
assets=manifest.get("assets",[])
if len(assets)!=len(EXPECTED) or tuple(x.get("name") for x in assets)!=EXPECTED:
    raise RuntimeError("Unexpected V6 inventory")
dest="/Game/AmberMarch/Meshes"
tools=unreal.AssetToolsHelpers.get_asset_tools()
paths=[]
for record in assets:
    name=record["name"]
    fbx=SRC/(name+".fbx")
    if not fbx.is_file() or fbx.stat().st_size!=record["bytes"] or \
       hashlib.sha256(fbx.read_bytes()).hexdigest()!=record["sha256"]:
        raise RuntimeError("FBX missing or checksum mismatch: "+name)
    game_path=dest+"/"+name
    if unreal.EditorAssetLibrary.does_asset_exist(game_path):
        raise RuntimeError("Refusing to replace pre-existing asset: "+game_path)
    paths.append((name,fbx,game_path))

verified=[]
for name,fbx,game_path in paths:
    task=unreal.AssetImportTask()
    task.filename=str(fbx)
    task.destination_path=dest
    task.destination_name=name
    task.automated=True
    task.save=True
    task.replace_existing=False
    opt=unreal.FbxImportUI()
    opt.import_mesh=True
    opt.import_as_skeletal=False
    opt.import_materials=False
    opt.import_textures=False
    opt.import_animations=False
    opt.automated_import_should_detect_type=False
    opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
    opt.static_mesh_import_data.combine_meshes=True
    opt.static_mesh_import_data.auto_generate_collision=False
    task.options=opt
    tools.import_asset_tasks([task])
    mesh=unreal.load_asset(game_path)
    if not isinstance(mesh,unreal.StaticMesh):
        raise RuntimeError("Import did not produce StaticMesh: "+name)
    slots=mesh.get_editor_property("static_materials")
    used=[]
    for idx,slot in enumerate(slots):
        key=str(slot.get_editor_property("material_slot_name"))
        material=unreal.load_asset("/Game/AmberMarch/Materials/M_"+key)
        if not isinstance(material,unreal.MaterialInterface):
            raise RuntimeError("Unmapped or unsupported palette slot %s/%s" % (name,key))
        mesh.set_material(idx,material)
        used.append(key)
    if len(slots)<3:
        raise RuntimeError("Unexpected low material diversity: "+name)
    # Keep collision disabled at the component level in-game; no generated
    # simple collision here. Never use these meshes as path authority.
    if not unreal.EditorAssetLibrary.save_loaded_asset(mesh):
        raise RuntimeError("Unreal save failed: "+name)
    bounds=mesh.get_bounds()
    extent=bounds.box_extent
    max_extent=max(abs(extent.x),abs(extent.y),abs(extent.z))
    if not (100.0 < max_extent < 1300.0):
        raise RuntimeError("Unreasonable imported world units for "+name+": "+str(max_extent))
    verified.append({
        "name":name,"game_asset_path":game_path,
        "size_x_cm":round(2.0*extent.x,1),
        "size_y_cm":round(2.0*extent.y,1),
        "size_z_cm":round(2.0*extent.z,1),
        "material_slots":used,
    })
out=ROOT/"Saved"/"QA"/"architecture_v6_import_receipt.json"
if out.exists():
    raise RuntimeError("Receipt already exists: cannot overwrite")
out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps({
    "status":"PASS_ARCHITECTURE_V6_IMPORT",
    "scope":"Additive editor-only import; no overwrite",
    "geometry_source":str(SPEC.relative_to(ROOT)).replace("\\","/"),
    "meshes":verified
},ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
unreal.log("ARCHITECTURE_V6_IMPORT_PASS meshes="+str(len(verified)))
