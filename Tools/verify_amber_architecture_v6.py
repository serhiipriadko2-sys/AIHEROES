"""Deterministic, read-only verification of ArchitectureV6 authoring receipts.

Optional --write-receipt creates exactly one new report, never overwrites it.
Does NOT claim an Unreal import or game-runtime visual acceptance.
"""
import argparse
import ast
import hashlib
import json
import struct
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
ART=ROOT/"ArtSource"/"AmberMarch"/"ArchitectureV6"
PACKAGE=ROOT/"Docs"/"ARCHITECTURE_V6_LOCAL_RECEIPT_2026-10-29.json"
NAMES=("SM_CitadelCrownV6","SM_FrontierGateV6","SM_ArchiveSanctumV6")
PREVIEWS=("citadel_preview.png","frontier_preview.png","archive_preview.png")
SCRIPTS=(
    "Tools/build_amber_architecture_v6.py",
    "Tools/finalize_amber_architecture_v6.py",
    "Tools/preview_amber_architecture_v6.py",
    "Tools/import_amber_architecture_v6.py",
    "Tools/verify_amber_architecture_v6.py"
)
SCRIPT_RUNTIME="Source/AIHeroes/Private/AIHeroesZoneExpeditionArt.cpp"
TEST_SOURCE="Source/AIHeroes/Private/Tests/AIHeroesArchitectureV6Tests.cpp"

def digest(path):
    if not path.is_file() or path.is_symlink():
        raise RuntimeError("Required regular file missing: "+str(path))
    payload=path.read_bytes()
    if not payload:
        raise RuntimeError("Zero-byte file: "+str(path))
    return {
        "path":path.relative_to(ROOT).as_posix(),
        "bytes":len(payload),
        "sha256":hashlib.sha256(payload).hexdigest()
    }

def png_dimensions(path):
    data=path.read_bytes()[:24]
    if len(data)<24 or data[:8]!=b"\x89PNG\r\n\x1a\n" or data[12:16]!=b"IHDR":
        raise RuntimeError("Bad PNG: "+str(path))
    return struct.unpack(">II",data[16:24])

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--write-receipt",action="store_true")
    args=parser.parse_args()

    manifest_path=ART/"asset_manifest.json"
    manifest=json.loads(manifest_path.read_text(encoding="utf-8"))
    models=manifest.get("assets",[])
    if len(models)!=3 or tuple(m["name"] for m in models)!=NAMES:
        raise RuntimeError("Unexpected V6 manifest inventory")
    seen=set()
    listing=[digest(manifest_path),digest(ART/"ArchitectureV6.blend")]
    total_tris=0
    for obj in models:
        if obj["name"] in seen:
            raise RuntimeError("Duplicate architecture name")
        seen.add(obj["name"])
        file=ART/(obj["name"]+".fbx")
        current=digest(file)
        if current["bytes"]!=obj["bytes"] or current["sha256"]!=obj["sha256"]:
            raise RuntimeError("FBX manifest drift: "+obj["name"])
        if not 100<int(obj["tris"])<16000:
            raise RuntimeError("Geometry budget failed: "+obj["name"])
        bounds=obj.get("bounds_blender_m",{})
        for axis,low,high in zip("XYZ",bounds.get("min",[]),bounds.get("max",[])):
            if not -9.0<float(low)<float(high)<12.0:
                raise RuntimeError("Unreasonable bounds "+obj["name"]+axis)
        if len(bounds.get("min",[]))!=3 or len(bounds.get("max",[]))!=3:
            raise RuntimeError("Bounds incomplete "+obj["name"])
        total_tris+=obj["tris"]
        listing.append(current)
    if total_tris>10000:
        raise RuntimeError("Total architectural tris over authoring budget")
    for name in PREVIEWS:
        image_path=ART/name
        width,height=png_dimensions(image_path)
        if (width,height)!=(800,450):
            raise RuntimeError("Wrong preview dimensions "+name)
        entry=digest(image_path)
        if entry["bytes"]<10000:
            raise RuntimeError("Preview too small "+name)
        entry["dimensions"]=[width,height]
        listing.append(entry)
    for relative in SCRIPTS:
        path=ROOT/relative
        ast.parse(path.read_text(encoding="utf-8"),filename=relative)
        listing.append(digest(path))
    for relative in (SCRIPT_RUNTIME,TEST_SOURCE):
        listing.append(digest(ROOT/relative))
    optional_assets=[
        ROOT/"Content"/"AmberMarch"/"Meshes"/(name+".uasset")
        for name in NAMES
    ]
    imported=[asset for asset in optional_assets if asset.is_file()]
    if len(imported) not in (0,3):
        raise RuntimeError("Unsafe partial architecture import")
    out={
        "schema":1,
        "status":"AUTHORING_PASS_ENGINE_IMPORT_PENDING" if not imported else
                 "AUTHORING_PASS_ENGINE_FILES_PRESENT_UNVERIFIED",
        "scope":"Three original V6 architecture meshes and static checks; no production deployment",
        "asset_count":3,
        "triangle_count":total_tris,
        "preview_count":3,
        "imported_asset_files":len(imported),
        "unreal_compile":"NOT_VERIFIED_FOR_THIS_SOURCE_SNAPSHOT",
        "runtime_gpu_visual":"NOT_VERIFIED_FOR_V6",
        "player_save_mutations":0,
        "provenance":listing,
    }
    if args.write_receipt:
        if PACKAGE.exists():
            raise RuntimeError("Receipt exists; refusing overwrite")
        PACKAGE.write_text(json.dumps(out,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
    print("ARCHITECTURE_V6_AUTHORING_PASS assets=%d tris=%d previews=%d files=%d imported=%d" %
          (out["asset_count"],out["triangle_count"],
           out["preview_count"],len(listing),len(imported)))
    print("STATUS="+out["status"])
    if args.write_receipt:
        print("RECEIPT="+str(PACKAGE))

if __name__=="__main__":
    main()
