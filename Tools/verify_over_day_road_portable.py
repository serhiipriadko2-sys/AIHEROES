#!/usr/bin/env python3
"""Portable, read-only route regression from the real Road V2 topology.

The road links are parsed from production AIHeroesTravelRules.cpp; the route
solver is the unchanged production AIHeroesAdventurePathCore.h. This proves
the actual *path core* behavior for the five-site test scenario without
compiling the entire Unreal DLL or touching GameMode, saves or GUI.
"""
from __future__ import annotations

import hashlib
import json
import re
import subprocess
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ROADS = ROOT / "Source/AIHeroes/Private/AIHeroesTravelRules.cpp"
CORE = ROOT / "Source/AIHeroes/Public/AIHeroesAdventurePathCore.h"
VS = Path(r"C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat")
IDS = ("HQ", "FOREST", "ORE", "OUTPOST", "RUINS")


def digest(file: Path) -> dict:
    payload = file.read_bytes()
    if not payload:
        raise RuntimeError(f"Empty input {file}")
    return {
        "path": file.relative_to(ROOT).as_posix(),
        "bytes": len(payload),
        "sha256": hashlib.sha256(payload).hexdigest(),
    }


def get_links() -> list[tuple[str, str, int]]:
    source = ROADS.read_text(encoding="utf-8-sig")
    links = re.findall(
        r'Road\(\s*TEXT\("([A-Z]+)"\)\s*,\s*TEXT\("([A-Z]+)"\)\s*,\s*(\d+)\s*\)',
        source,
    )
    values = [(a, b, int(cost)) for a, b, cost in links]
    if len(values) != 5 or any(
        a not in IDS or b not in IDS or a == b or cost <= 0 or cost > 4
        for a, b, cost in values
    ):
        raise RuntimeError("Unexpected Road V2 topology. Freeze before regression.")
    if len({frozenset((a, b)) for a, b, _ in values}) != 5:
        raise RuntimeError("Duplicate links in Road V2")
    expected = {
        frozenset(("HQ", "FOREST")): 2,
        frozenset(("HQ", "ORE")): 2,
        frozenset(("FOREST", "OUTPOST")): 1,
        frozenset(("ORE", "RUINS")): 1,
        frozenset(("OUTPOST", "RUINS")): 2,
    }
    if {frozenset((a, b)): cost for a, b, cost in values} != expected:
        raise RuntimeError("Road V2 prices changed; update explicit gameplay tests")
    return values


TEST = r"""
#include "AIHeroesAdventurePathCore.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace AIHeroesAdventureCore;
int Checks=0;
void Check(bool Good,const char* Message) {
    if (!Good) throw std::runtime_error(Message);
    ++Checks;
}
int main() {
    try {
        Graph G;
        G.Revision=2;
        G.Nodes={
            {"HQ",true,true,false},
            {"FOREST",true,true,false},
            {"ORE",true,true,false},
            {"OUTPOST",true,true,true},
            {"RUINS",true,true,true}
        };
        auto Road=[&G](const std::string& A,const std::string& B,int Cost) {
            G.Links.push_back({A,B,Cost,true});
            G.Links.push_back({B,A,Cost,true});
        };
GENERATED_ROADS
        const auto Long=Find(G,"ORE","OUTPOST",4);
        Check(Long.State==Status::NeedsMoreMoves,
            "Five-move crossmap route must not be advertised as ready");
        Check(Long.Cost==5 && Long.Missing==1,
            "Exact full-day price must be 5 with one missing");
        Check(Long.Battle,"Guard is present at the destination");
        Check(Long.NodeIndices.size()==4,
            "Long path must include both intermediate nodes");
        Check(Long.NodeIndices[0]==2 && Long.NodeIndices[1]==0 &&
              Long.NodeIndices[2]==1 && Long.NodeIndices[3]==3,
              "Canonical route must cross owned HQ and FOREST");
        Check(Find(G,"ORE","OUTPOST",5).State==Status::Ready,
            "Five-move quote is valid only when budget could be five");
        Check(Find(G,"ORE","HQ",4).State==Status::Ready,
            "HQ is a valid short staging waypoint");
        Check(Find(G,"ORE","HQ",4).Cost==2,
            "First owned staging leg costs two moves");
        Check(Find(G,"ORE","FOREST",4).State==Status::Ready,
            "Furthest owned intermediate is also a legal four-move stop");
        Check(Find(G,"ORE","FOREST",4).Cost==4,
            "Second owned staging leg costs exactly four moves");
        Check(Find(G,"ORE","HQ",1).State==Status::NeedsMoreMoves,
            "Insufficient remaining moves for short leg: End Day can help");

        // Unseen, guarded or malformed graph cannot create an invented stage.
        G.Nodes[1].Guard=true;
        Check(Find(G,"ORE","OUTPOST",4).State==Status::Unreachable,
            "Cannot transit the guarded forest");
        G.Nodes[1].Guard=false;G.Nodes[1].Discovered=false;
        Check(Find(G,"ORE","OUTPOST",4).State==Status::Unreachable,
            "Hidden forest cannot be used as a secret waypoint");
        G.Nodes[1].Discovered=true;
        G.Links.push_back(G.Links.back());
        Check(Find(G,"ORE","OUTPOST",4).State==Status::InvalidInput,
            "Invalid duplicate topology must fail closed");
        std::cout<<"ROAD_V2_PORTABLE_CORE_PASS checks="<<Checks<<"\n";
        return 0;
    } catch(const std::exception& E) {
        std::cerr<<"ROAD_V2_PORTABLE_CORE_FAIL: "<<E.what()<<"\n";
        return 1;
    }
}
"""


def main() -> None:
    if not VS.is_file():
        raise RuntimeError("Missing VS environment")
    roads = get_links()
    road_src = "\n".join(
        f'        Road("{a}","{b}",{cost});'
        for a, b, cost in roads
    )
    content = TEST.replace("GENERATED_ROADS", road_src)
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    out = ROOT / "Saved" / "QA" / ("RoadV2Portable_" + stamp)
    out.mkdir(parents=True, exist_ok=False)
    src = out / "road_core_test.cpp"
    exe = out / "road_core_test.exe"
    batch = out / "compile.cmd"
    src.write_text(content, encoding="utf-8")
    batch.write_text(
        '@echo off\r\n'
        f'call "{VS}" -no_logo -arch=x64 >nul 2>&1\r\n'
        'if errorlevel 1 exit /b 50\r\n'
        f'cl /nologo /std:c++17 /EHsc /W4 /WX '
        f'/I"{CORE.parent}" /Fe:"{exe}" "{src}" /link /INCREMENTAL:NO\r\n'
        'exit /b %errorlevel%\r\n',
        encoding="ascii", newline=""
    )
    compiled = subprocess.run(
        ["cmd.exe", "/d", "/c", str(batch)], cwd=out,
        capture_output=True, text=True, errors="replace", timeout=100,
    )
    compiler_log = out / "compile_output.txt"
    compiler_log.write_text(
        compiled.stdout + compiled.stderr, encoding="utf-8",
    )
    if compiled.returncode != 0 or not exe.is_file():
        raise RuntimeError(f"MSVC compile failed: {compiler_log}")
    executed = subprocess.run(
        [str(exe)], cwd=out, capture_output=True,
        text=True, errors="replace", timeout=20,
    )
    execution_log = out / "execution_output.txt"
    execution_log.write_text(
        executed.stdout + executed.stderr, encoding="utf-8",
    )
    if executed.returncode or "ROAD_V2_PORTABLE_CORE_PASS" not in executed.stdout:
        raise RuntimeError(f"Road V2 assertions failed: {execution_log}")
    receipt = {
        "schema": "aiheroes.road-v2-portable-core.v1",
        "status": "PASS_PORTABLE_ROAD_V2_NOT_UNREAL",
        "output": executed.stdout.strip(),
        "production_source": [digest(CORE), digest(ROADS)],
        "evidence": [
            digest(path)
            for path in (src, exe, batch, compiler_log, execution_log)
        ],
        "read_only_game_state": True,
        "limitations": [
            "Road prices parsed from the current production TravelRules file.",
            "Route decisions compiled against actual header-only production path core.",
            "Guard/discovery scenario is a fixture; no Unreal Actor state is used.",
            "Does not verify Unreal build, SaveGame or live game presentation.",
        ],
    }
    manifest = out / "receipt.json"
    manifest.write_text(json.dumps(receipt, ensure_ascii=False, indent=2) + "\n",
                        encoding="utf-8")
    if json.loads(manifest.read_text(encoding="utf-8")) != receipt:
        raise RuntimeError("Receipt readback mismatch")
    print(executed.stdout.strip())
    print(f"ROAD_PORTABLE_RECEIPT={manifest}")
    print(f"RECEIPT_BYTES={manifest.stat().st_size} SHA256={digest(manifest)['sha256']}")


if __name__ == "__main__":
    main()
