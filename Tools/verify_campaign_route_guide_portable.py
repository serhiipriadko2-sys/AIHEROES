#!/usr/bin/env python3
"""Compile/run the current AIHeroesCampaignGuide C++ against minimal type shims.

This verifies the EXACT production Guide::Build and its context declaration
without the huge Unreal precompiled headers. It is explicitly NOT a UE build
or a proof that live-game HUD wiring is correct. Read-only with respect to
Source, Content, SaveGames and the active VS Code session.
"""
from __future__ import annotations

import hashlib
import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE_H = ROOT / "Source/AIHeroes/Public/AIHeroesCampaignGuide.h"
SOURCE_CPP = ROOT / "Source/AIHeroes/Private/AIHeroesCampaignGuide.cpp"
EXPLORER = ROOT / "Source/AIHeroes/Private/AIHeroesExplorerHUD.cpp"
MSVC_ENV = Path(
    r"C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"
)

STUBS = r"""
#define WIN32_LEAN_AND_MEAN
#include <algorithm>
#include <cstdarg>
#include <cwchar>
#include <iostream>
#include <stdexcept>
#include <string>

#define TEXT(X) L##X
#define AIHEROES_API
using int32 = int;
class FName {
public:
    FName() = default;
    FName(const wchar_t* Name) : Name_(Name ? Name : L"") {}
    bool IsNone() const { return Name_.empty(); }
    bool operator==(const FName& Other) const { return Name_==Other.Name_; }
    bool operator!=(const FName& Other) const { return !(*this==Other); }
private:
    std::wstring Name_;
};
const FName NAME_None;
class FString {
public:
    FString() = default;
    FString(const wchar_t* Text) : Text_(Text ? Text : L"") {}
    explicit FString(const std::wstring& Text) : Text_(Text) {}
    const wchar_t* operator*() const { return Text_.c_str(); }
    bool IsEmpty() const { return Text_.empty(); }
    bool Contains(const wchar_t* Text) const {
        return Text_.find(Text) != std::wstring::npos;
    }
    FString operator+(const FString& Other) const {
        return FString(Text_+Other.Text_);
    }
    static FString Printf(const wchar_t* Format, ...) {
        wchar_t Buffer[1024] = {};
        va_list Args;
        va_start(Args, Format);
        const int N = vswprintf_s(Buffer, 1024, Format, Args);
        va_end(Args);
        if (N < 0) throw std::runtime_error("Printf error");
        return FString(Buffer);
    }
private:
    std::wstring Text_;
};
struct FMath {
    template<typename T>
    static T Clamp(T V, T Min, T Max) {
        return (std::min)((std::max)(V, Min), Max);
    }
};
struct FAIHeroesCampaignObjectives { static constexpr int32 GoalCount=4; };
struct FAIHeroesCampaignProgress {
    int32 Total=4;
    int32 Completed=0;
    FName NextFocusId;
    bool bVictory=false;
};
"""
TESTS = r"""
int Checks=0;
void Check(bool Result, const char* Label) {
    if (!Result) throw std::runtime_error(Label);
    ++Checks;
}
int main() {
    try {
        FAIHeroesCampaignProgress P;
        P.NextFocusId=FName(TEXT("FOREST"));
        FAIHeroesCampaignGuideContext C;
        C.SelectedId=FName(TEXT("FOREST"));
        C.bHasArmy=true;
        C.bTargetDiscovered=true;
        C.bNeedsMoreMoves=true;
        C.MissingMoves=1;
        C.RouteTotalCost=3;
        C.FullDayMoveBudget=4;

        auto Advice=FAIHeroesCampaignGuide::Build(P,C);
        Check(Advice.NextStep.Contains(TEXT("закончите день")),
            "Short route must offer legal next-day retry");
        Check(Advice.Detail.Contains(TEXT("1")),
            "Missing-moves amount must be preserved");

        C.RouteTotalCost=5;
        C.IntermediateOwnedSite=FName(TEXT("HQ"));
        Advice=FAIHeroesCampaignGuide::Build(P,C);
        Check(Advice.NextStep.Contains(TEXT("Через Цитадель Искры")),
            "Route longer than whole day requires staging");
        Check(Advice.NextStep.Contains(TEXT("Цитадель Искры")),
            "Discovered, owned intermediate waypoint must be named");
        Check(Advice.Detail.Contains(TEXT("5")),
            "Route cost must not be hidden");
        Check(Advice.Detail.Contains(TEXT("4")),
            "Full-day budget must be visible");
        Check(!Advice.NextStep.Contains(TEXT("закончите день")),
            "Cannot recommend endless End Day");
        Check(!Advice.Detail.Contains(TEXT("Конец хода")),
            "End Day cannot solve a five-move direct trip");

        C.IntermediateOwnedSite=NAME_None;
        Advice=FAIHeroesCampaignGuide::Build(P,C);
        Check(Advice.Detail.Contains(TEXT("Ищите свою точку")),
            "No verified waypoint must produce honest generic fallback");
        Check(!Advice.Detail.Contains(TEXT("Цитадель")),
            "Unknown context must not guess a site");

        C.IntermediateOwnedSite=FName(TEXT("SECRET_GUARD"));
        Advice=FAIHeroesCampaignGuide::Build(P,C);
        Check(!Advice.Detail.Contains(TEXT("SECRET")),
            "Unrecognized IDs must never appear in guidance");

        C.bTravelMoving=true;
        Advice=FAIHeroesCampaignGuide::Build(P,C);
        Check(Advice.NextStep.Contains(TEXT("пути")),
            "Moving hero must not receive a second order");

        C.bTravelMoving=false;
        C.bHasArmy=false;
        Advice=FAIHeroesCampaignGuide::Build(P,C);
        Check(Advice.NextStep.Contains(TEXT("наймите")),
            "Empty army overrides route advice");

        C.bHasArmy=true;C.bTargetDiscovered=false;
        Advice=FAIHeroesCampaignGuide::Build(P,C);
        Check(Advice.NextStep.Contains(TEXT("Разведайте")),
            "Fog remains authoritative");
        Check(!Advice.Detail.Contains(TEXT("SECRET")),
            "Fog advice cannot reveal a hidden site");

        C.bTargetDiscovered=true;C.SelectedId=FName(TEXT("ORE"));
        Advice=FAIHeroesCampaignGuide::Build(P,C);
        Check(Advice.NextStep.Contains(TEXT("Лесопилка")),
            "Wrong selection must direct player to canonical goal");

        C.SelectedId=FName(TEXT("FOREST"));C.bNeedsMoreMoves=false;
        Advice=FAIHeroesCampaignGuide::Build(P,C);
        Check(Advice.NextStep.Contains(TEXT("закрыт")),
            "Unavailable route must not promise movement");

        C.bRouteReady=true;
        Advice=FAIHeroesCampaignGuide::Build(P,C);
        Check(Advice.NextStep.Contains(TEXT("подтвердите")),
            "Reachable target retains human confirmation");

        P.bVictory=true;
        Advice=FAIHeroesCampaignGuide::Build(P,C);
        Check(Advice.NextStep.Contains(TEXT("Сохраните")),
            "Victory is stable and does not ask to march");

        P.bVictory=false;P.Total=0;
        Advice=FAIHeroesCampaignGuide::Build(P,C);
        Check(Advice.FocusId.IsNone(),
            "Invalid goal set must fall back without a phantom target");
        std::cout << "GUIDE_PORTABLE_METHODS_PASS checks=" << Checks << "\n";
        return 0;
    } catch(const std::exception& E) {
        std::cerr << "GUIDE_PORTABLE_METHODS_FAIL: " << E.what() << "\n";
        return 1;
    }
}
"""


def file_receipt(path: Path) -> dict:
    payload = path.read_bytes()
    if not payload:
        raise RuntimeError(f"Empty file: {path}")
    return {
        "path": path.relative_to(ROOT).as_posix(),
        "bytes": len(payload),
        "sha256": hashlib.sha256(payload).hexdigest(),
    }


def main() -> None:
    if not MSVC_ENV.is_file():
        raise RuntimeError("Expected Visual Studio development command missing")
    # Exact primary game-file bodies. No copied logic in test fixtures.
    header = "\n".join(
        ln for ln in SOURCE_H.read_text(encoding="utf-8-sig").splitlines()
        if not ln.lstrip().startswith(("#pragma", "#include"))
    )
    guide = "\n".join(
        ln for ln in SOURCE_CPP.read_text(encoding="utf-8-sig").splitlines()
        if not ln.lstrip().startswith("#include")
    )
    if "FAIHeroesCampaignGuide::Build" not in guide or "FullDayMoveBudget" not in guide:
        raise RuntimeError("Unexpected production Guide::Build source contract")
    # Wiring checks ensure actual HUD builds context from the real path and
    # validates ownership/discovery instead of trusting a label string.
    hud = EXPLORER.read_text(encoding="utf-8-sig")
    for invariant in (
        "GuideContext.RouteTotalCost=Valid?Path.TotalCost:0",
        "Path.Nodes[I]",
        "Stop->bOwned",
        "G.IsNodeDiscovered(StopId)",
        "GuideContext.IntermediateOwnedSite=StopId",
        "Path.TotalCost>GuideContext.FullDayMoveBudget",
    ):
        if invariant not in hud:
            raise RuntimeError("Production HUD context wiring changed: " + invariant)

    out = ROOT / "Saved" / "QA" / (
        "CampaignRoutePortable_" + datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    )
    out.mkdir(parents=True, exist_ok=False)
    cpp = out / "actual_guide_test.cpp"
    exe = out / "actual_guide_test.exe"
    batch = out / "compile.cmd"
    cpp.write_text(
        STUBS
        + '\n#line 1 "AIHeroesCampaignGuide.h (production)"\n'
        + header
        + '\n#line 1 "AIHeroesCampaignGuide.cpp (production)"\n'
        + guide
        + '\n#line 1 "PortableGuideAssertions"\n'
        + TESTS,
        encoding="utf-8",
    )
    batch.write_text(
        '@echo off\r\n'
        f'call "{MSVC_ENV}" -no_logo -arch=x64 >nul 2>&1\r\n'
        'if errorlevel 1 exit /b 50\r\n'
        f'cl /nologo /std:c++17 /EHsc /W4 /WX /Zc:__cplusplus '
        f'/Fe:"{exe}" "{cpp}" /link /INCREMENTAL:NO\r\n'
        'exit /b %errorlevel%\r\n',
        encoding="ascii",
        newline="",
    )
    build = subprocess.run(
        ["cmd.exe", "/d", "/c", str(batch)],
        cwd=out,
        capture_output=True,
        text=True,
        timeout=90,
        errors="replace",
    )
    compile_log = out / "compile_output.txt"
    compile_log.write_text(build.stdout + build.stderr, encoding="utf-8")
    if build.returncode or not exe.is_file():
        raise RuntimeError(
            f"Portable MSVC failed code={build.returncode} log={compile_log}"
        )
    run = subprocess.run(
        [str(exe)], cwd=out, capture_output=True, text=True,
        timeout=15, errors="replace",
    )
    output = out / "execution_output.txt"
    output.write_text(run.stdout + run.stderr, encoding="utf-8")
    if run.returncode or "GUIDE_PORTABLE_METHODS_PASS" not in run.stdout:
        raise RuntimeError(f"Guide assertions failed code={run.returncode}, see {output}")

    receipt = {
        "schema": "aiheroes.guide-portable-msvc.v1",
        "status": "PASS_PORTABLE_GUIDE_METHOD_NOT_UNREAL",
        "output": run.stdout.strip(),
        "validation_boundary": [
            "Uses exact production CampaignGuide.h and CampaignGuide.cpp bodies.",
            "CoreMinimal/Unreal types replaced by small C++ shims.",
            "Checks source wiring of ExplorerHUD, not runtime actor graph.",
            "Not an Unreal build, automation run, GPU test or full gameplay pass.",
        ],
        "production_inputs": [
            file_receipt(path) for path in (SOURCE_H, SOURCE_CPP, EXPLORER)
        ],
        "evidence": [
            file_receipt(path)
            for path in (cpp, exe, batch, compile_log, output)
        ],
    }
    manifest = out / "receipt.json"
    manifest.write_text(
        json.dumps(receipt, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )
    readback = json.loads(manifest.read_text(encoding="utf-8"))
    if readback != receipt:
        raise RuntimeError("Receipt readback mismatch")
    print(run.stdout.strip())
    print(f"GUIDE_PORTABLE_RECEIPT={manifest}")
    print(f"RECEIPT_BYTES={manifest.stat().st_size} sha256={file_receipt(manifest)['sha256']}")


if __name__ == "__main__":
    main()
