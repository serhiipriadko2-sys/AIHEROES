#!/usr/bin/env python3
"""Portable MSVC gate for the actual victory UI method bodies (no Unreal PCH).

This is deliberately NOT an Unreal build/test or GPU acceptance. It extracts
the production FAIHeroesExplorerLayout::Frame and
FAIHeroesVictoryPresentation::{IsAuthoritativeVictory,Banner,HitTest} method
bodies, compiles them against tiny, documented type shims with /W4 /WX, and
executes boundary tests. No production source or user saves are changed.
"""
from __future__ import annotations

import hashlib
import json
import re
import subprocess
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE_FRAME = ROOT / "Source/AIHeroes/Private/AIHeroesExplorerLayout.cpp"
SOURCE_VICTORY = ROOT / "Source/AIHeroes/Private/AIHeroesVictoryPresentation.cpp"
SOURCE_HEADER = ROOT / "Source/AIHeroes/Public/AIHeroesVictoryPresentation.h"
VSDEVCMD = Path(
    r"C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"
)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_methods() -> tuple[str, str]:
    frame = SOURCE_FRAME.read_text(encoding="utf-8-sig")
    victory = SOURCE_VICTORY.read_text(encoding="utf-8-sig")
    start = frame.find("FAIHeroesExplorerFrame FAIHeroesExplorerLayout::Frame(")
    end = frame.find("\nTArray<int32> FAIHeroesExplorerLayout::Actions(", start)
    if start < 0 or end < 0:
        raise RuntimeError("The production layout method changed; do not simulate it")
    frame = frame[start:end]
    victory = "\n".join(
        line for line in victory.splitlines()
        if not line.lstrip().startswith("#include")
    )
    expected = (
        "FAIHeroesVictoryPresentation::IsAuthoritativeVictory(",
        "FAIHeroesVictoryPresentation::Banner(",
        "FAIHeroesVictoryPresentation::HitTest(",
    )
    if any(victory.count(marker) != 1 for marker in expected):
        raise RuntimeError("The production victory functions changed")
    if "#include" in victory or "GEngine" in victory:
        raise RuntimeError("Portable method body is no longer self-contained")
    return frame, victory


STUB = r"""
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

using int32 = int;
constexpr int32 INDEX_NONE = -1;

struct FVector2D { float X=0.f, Y=0.f; };
struct FAIHeroesHUDActionRect {
    float X=0.f, Y=0.f, W=0.f, H=0.f;
    bool Contains(const FVector2D& P) const {
        return W > 0.f && H > 0.f && P.X >= X && P.X <= X+W
            && P.Y >= Y && P.Y <= Y+H;
    }
};
struct FAIHeroesExplorerFrame {
    FAIHeroesHUDActionRect Header,Dock,Inspector,Menu,Map,Mission,Overview,Hero;
    bool bWide=false, bSupported=false;
};
struct FMath {
    template<class T> static T Max(T a, T b) { return std::max(a,b); }
    template<class T> static T Min(T a, T b) { return std::min(a,b); }
    static bool IsFinite(float x) { return std::isfinite(x); }
};
struct FAIHeroesCampaignGoal {
    int32 Current=0, Required=1;
    bool IsComplete() const { return Required>0 && Current>=Required; }
};
struct GoalList : std::vector<FAIHeroesCampaignGoal> {
    int32 Num() const { return static_cast<int32>(size()); }
};
struct FAIHeroesCampaignProgress {
    GoalList Goals;
    int32 Completed=0, Total=0;
    bool bVictory=false;
};
struct FAIHeroesCampaignObjectives { static constexpr int32 GoalCount=4; };
struct FAIHeroesExplorerLayout {
    static FAIHeroesExplorerFrame Frame(const FVector2D& V);
};
struct FAIHeroesVictoryPresentation {
    static constexpr int32 OpenGoalsAction=202;
    static bool IsAuthoritativeVictory(
        const FAIHeroesCampaignProgress& Progress);
    static FAIHeroesHUDActionRect Banner(
        const FAIHeroesCampaignProgress& Progress,
        const FVector2D& Viewport, bool bUtilityMenuOpen);
    static int32 HitTest(
        const FAIHeroesCampaignProgress& Progress,
        const FVector2D& Viewport, bool bUtilityMenuOpen,
        const FVector2D& ScreenPoint);
};
"""

TESTS = r"""
int Checks=0;
void Check(bool Good, const char* Message) {
    if (!Good) throw std::runtime_error(Message);
    ++Checks;
}
bool Intersects(const FAIHeroesHUDActionRect& A,
                const FAIHeroesHUDActionRect& B) {
    return A.W > 0.f && B.W > 0.f &&
        A.X < B.X+B.W && A.X+A.W > B.X &&
        A.Y < B.Y+B.H && A.Y+A.H > B.Y;
}
int main() {
    try {
        FAIHeroesCampaignProgress Won;
        Won.Completed=4; Won.Total=4; Won.bVictory=true;
        Won.Goals.push_back({2,2});
        Won.Goals.push_back({2,2});
        Won.Goals.push_back({3,3});
        Won.Goals.push_back({2,2});
        Check(FAIHeroesVictoryPresentation::IsAuthoritativeVictory(Won),
            "Canonical goals should show victory");

        for (FVector2D V : {FVector2D{1150.f,640.f},
                             FVector2D{1280.f,720.f},
                             FVector2D{1600.f,900.f}}) {
            const auto Layout=FAIHeroesExplorerLayout::Frame(V);
            const auto B=FAIHeroesVictoryPresentation::Banner(Won,V,false);
            Check(B.W>=450.f && B.H>=80.f,"Wide victory plaque invisible");
            Check(B.X>=Layout.Map.X && B.Y>=Layout.Map.Y
                && B.X+B.W<=Layout.Map.X+Layout.Map.W
                && B.Y+B.H<=Layout.Map.Y+Layout.Map.H,
                "Plaque extends outside playable map");
            Check(!Intersects(B,Layout.Mission) &&
                  !Intersects(B,Layout.Inspector) &&
                  !Intersects(B,Layout.Dock) &&
                  !Intersects(B,Layout.Overview),
                  "Plaque covers other controls");
            FVector2D Center{B.X+B.W/2,B.Y+B.H/2};
            Check(FAIHeroesVictoryPresentation::HitTest(Won,V,false,Center)
                == FAIHeroesVictoryPresentation::OpenGoalsAction,
                "Click must open existing Goals action 202");
            Check(FAIHeroesVictoryPresentation::HitTest(
                Won,V,false,FVector2D{B.X-10.f,B.Y})==INDEX_NONE,
                "Outside click must not open goals");
            Check(FAIHeroesVictoryPresentation::HitTest(
                Won,V,true,Center)==INDEX_NONE,
                "Menu-open cannot have invisible hit-target");
            Check(Layout.Mission.Y==Layout.Map.Y,
                "Current layout no longer aligns mission/map tops");
        }
        for (FVector2D V : {FVector2D{960.f,540.f},
                             FVector2D{640.f,520.f},
                             FVector2D{480.f,800.f},
                             FVector2D{320.f,480.f}}) {
            Check(FAIHeroesVictoryPresentation::Banner(Won,V,false).W==0.f,
                "Narrow viewport should use existing objective UI");
            Check(FAIHeroesVictoryPresentation::HitTest(Won,V,false,
                FVector2D{200.f,200.f})==INDEX_NONE,
                "Narrow viewport must not trap map clicks");
        }
        auto Invalid=Won; Invalid.Completed=3;
        Check(!FAIHeroesVictoryPresentation::IsAuthoritativeVictory(Invalid),
            "Forged completion count accepted");
        Invalid=Won; Invalid.Goals[2].Current=0;
        Check(!FAIHeroesVictoryPresentation::IsAuthoritativeVictory(Invalid),
            "Incomplete protocol accepted");
        Invalid=Won; Invalid.Total=3;
        Check(!FAIHeroesVictoryPresentation::IsAuthoritativeVictory(Invalid),
            "Wrong goal count accepted");
        Invalid=Won; Invalid.bVictory=false;
        Check(!FAIHeroesVictoryPresentation::IsAuthoritativeVictory(Invalid),
            "False bVictory accepted");
        Check(FAIHeroesVictoryPresentation::Banner(
            Won,FVector2D{std::numeric_limits<float>::quiet_NaN(),720.f},
            false).W==0.f,"Invalid viewport produced geometry");
        Check(FAIHeroesVictoryPresentation::Banner(
            Won,FVector2D{1280.f,-1.f},false).W==0.f,
            "Negative height produced geometry");
        std::cout<<"VICTORY_PORTABLE_METHODS_PASS checks="<<Checks<<'\n';
        return 0;
    } catch(const std::exception& E) {
        std::cerr<<"VICTORY_PORTABLE_METHODS_FAIL: "<<E.what()<<'\n';
        return 1;
    }
}
"""


def main() -> None:
    if not VSDEVCMD.is_file():
        raise RuntimeError("MSVC 18 VsDevCmd not found")
    frame, victory = source_methods()
    run_id = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    out = ROOT / "Saved" / "QA" / ("VictoryPortable_" + run_id)
    out.mkdir(parents=True, exist_ok=False)
    cpp = out / "source_methods_test.cpp"
    exe = out / "source_methods_test.exe"
    batch = out / "compile.cmd"
    cpp.write_text(
        STUB + "\n#line 1 \"FAIHeroesExplorerLayout.Frame(production)\"\n"
        + frame + "\n#line 1 \"FAIHeroesVictoryPresentation(production)\"\n"
        + victory + "\n#line 1 \"VictoryPortableTests\"\n"
        + TESTS,
        encoding="utf-8",
    )
    batch.write_text(
        '@echo off\r\n'
        f'call "{VSDEVCMD}" -no_logo -arch=x64 >nul 2>&1\r\n'
        'if errorlevel 1 exit /b 50\r\n'
        f'cl /nologo /std:c++17 /EHsc /W4 /WX /Zc:__cplusplus '
        f'/Fe:"{exe}" "{cpp}" /link /INCREMENTAL:NO\r\n'
        'exit /b %errorlevel%\r\n',
        encoding="ascii",
        newline="",
    )
    compiled = subprocess.run(
        ["cmd.exe", "/d", "/c", str(batch)],
        cwd=out,
        capture_output=True,
        text=True,
        timeout=100,
        errors="replace",
    )
    (out / "compiler_output.txt").write_text(
        compiled.stdout + compiled.stderr,
        encoding="utf-8",
    )
    if compiled.returncode or not exe.is_file():
        raise RuntimeError(
            f"Portable MSVC compile FAILED ({compiled.returncode}): "
            f"{out / 'compiler_output.txt'}"
        )
    executed = subprocess.run(
        [str(exe)],
        cwd=out,
        capture_output=True,
        text=True,
        timeout=20,
        errors="replace",
    )
    (out / "test_output.txt").write_text(
        executed.stdout + executed.stderr, encoding="utf-8"
    )
    if executed.returncode or "VICTORY_PORTABLE_METHODS_PASS" not in executed.stdout:
        raise RuntimeError(
            f"Portable actual-method assertions FAILED ({executed.returncode}): "
            f"{out / 'test_output.txt'}"
        )
    receipt = {
        "status": "PASS_PORTABLE_METHOD_TEST_NOT_UNREAL",
        "limitations": [
            "Unreal headers are replaced by narrow shims; not a UE compile.",
            "Tests the exact current C++ method bodies, not GPU rendering.",
            "No game SaveGames or live UI widgets are accessed.",
        ],
        "production_source": [
            {"path": p.relative_to(ROOT).as_posix(), "bytes": p.stat().st_size,
             "sha256": sha(p)}
            for p in (SOURCE_FRAME, SOURCE_VICTORY, SOURCE_HEADER)
        ],
        "evidence": [
            {"path": p.relative_to(ROOT).as_posix(), "bytes": p.stat().st_size,
             "sha256": sha(p)}
            for p in (cpp, exe, batch, out / "compiler_output.txt",
                      out / "test_output.txt")
        ],
        "output": executed.stdout.strip(),
    }
    receipt_path = out / "receipt.json"
    receipt_path.write_text(
        json.dumps(receipt, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    confirmed = json.loads(receipt_path.read_text(encoding="utf-8"))
    if confirmed != receipt:
        raise RuntimeError("Receipt read-back mismatch")
    print(executed.stdout.strip())
    print(f"VICTORY_PORTABLE_RECEIPT={receipt_path}")
    print(f"RECEIPT_BYTES={receipt_path.stat().st_size} SHA256={sha(receipt_path)}")


if __name__ == "__main__":
    main()
