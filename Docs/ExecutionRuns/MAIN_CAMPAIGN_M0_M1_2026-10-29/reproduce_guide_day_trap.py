"""Low-memory diagnostic of actual guide code plus actual path kernel.

Not Unreal, not engine-adapter/GUI/save acceptance. The real Guide header/body
is compiled against explicit FString/FName/FMath/Progress test shims. The path
kernel is included directly. No production file is changed. Expected initial
scenario exit1 is a genuine failed instruction assertion, not a compiler error.
"""
from pathlib import Path
import argparse
import datetime
import hashlib
import json
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
INPUTS = (
    "Source/AIHeroes/Public/AIHeroesCampaignGuide.h",
    "Source/AIHeroes/Private/AIHeroesCampaignGuide.cpp",
    "Source/AIHeroes/Public/AIHeroesCampaignObjectives.h",
    "Source/AIHeroes/Public/AIHeroesAdventurePathCore.h",
    "Source/AIHeroes/Private/AIHeroesTravelRules.cpp",
)
VC = Path(r"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat")


def digest(relative):
    p = ROOT / relative
    b = p.read_bytes()
    return {"path": relative, "bytes": len(b), "sha256": hashlib.sha256(b).hexdigest()}


def body(path):
    text = path.read_text(encoding="utf-8-sig")
    lines = []
    for line in text.splitlines():
        if line.startswith(("#include", "#pragma once")):
            continue
        if line.lstrip().startswith("#"):
            raise RuntimeError("Production conditional changed; do not simulate it: " + str(path))
        lines.append(line)
    return "\n".join(lines)


STUB = r'''
#include <algorithm>
#include <cstdarg>
#include <cwchar>
#include <cwctype>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include "AIHeroesAdventurePathCore.h"
using int32=int;
#define TEXT(x) L##x
#define AIHEROES_API
struct FString {
    std::wstring Value;
    FString()=default;
    FString(const wchar_t* S):Value(S){}
    FString(std::wstring S):Value(std::move(S)){}
    bool IsEmpty()const{return Value.empty();}
    bool Contains(const wchar_t* S) const {return Value.find(S)!=std::wstring::npos;}
    const wchar_t* operator*() const {return Value.c_str();}
    static FString Printf(const wchar_t* Format,...) {
        wchar_t Buffer[2048]{};va_list Args;va_start(Args,Format);
        _vsnwprintf_s(Buffer,2048,_TRUNCATE,Format,Args);va_end(Args);
        return FString(Buffer);
    }
    friend FString operator+(const FString& A,const wchar_t* B){return FString(A.Value+B);}
    friend FString operator+(const FString& A,const FString& B){return FString(A.Value+B.Value);}
};
struct FName {
    std::wstring Value;
    FName()=default;
    FName(const wchar_t* S):Value(S){
        for(auto& C:Value)C=static_cast<wchar_t>(std::towlower(C));
    }
    bool IsNone()const{return Value.empty();}
    FString ToString()const{return FString(Value);}
    friend bool operator==(const FName& A,const FName& B){return A.Value==B.Value;}
    friend bool operator!=(const FName& A,const FName& B){return !(A==B);}
};
const FName NAME_None{};
struct FMath {
    template<class T>static T Clamp(T V,T Low,T High){return std::clamp(V,Low,High);}
};
struct FAIHeroesCampaignProgress {
    int32 Completed=0,Total=4;bool bVictory=false;FName NextFocusId;
};
struct FAIHeroesCampaignObjectives{static constexpr int32 GoalCount=4;};
'''

SCENARIO = r'''
template<class T,class=void>struct HasCostEvidence:std::false_type{};
template<class T>struct HasCostEvidence<T,std::void_t<
    decltype(std::declval<T&>().RouteTotalCost),
    decltype(std::declval<T&>().FullDayMoveBudget),
    decltype(std::declval<T&>().IntermediateOwnedSite)>>:std::true_type{};
template<class T>void SupplyFacts(T& C,int32 Cost){
    (void)C;(void)Cost;
    if constexpr(HasCostEvidence<T>::value){
        C.RouteTotalCost=Cost;C.FullDayMoveBudget=4;
        C.IntermediateOwnedSite=FName(TEXT("HQ"));
    }
}
int main(){
    AIHeroesAdventureCore::Graph G;
    G.Nodes={{"HQ",true,true,false},{"FOREST",true,true,false},
             {"ORE",true,true,false},{"OUTPOST",true,true,true},
             {"RUINS",true,true,true}};
    auto Road=[&G](const char* A,const char* B,int32 Cost){
        G.Links.push_back({A,B,Cost,true});G.Links.push_back({B,A,Cost,true});
    };
    // __ROADS__
    const auto Full=AIHeroesAdventureCore::Find(G,"ORE","OUTPOST",4);
    const auto Tomorrow=AIHeroesAdventureCore::Find(G,"ORE","OUTPOST",4);
    const auto Stop=AIHeroesAdventureCore::Find(G,"ORE","HQ",4);
    if(Full.State!=AIHeroesAdventureCore::Status::NeedsMoreMoves || Full.Cost!=5 ||
       Tomorrow.Cost!=5 || Stop.State!=AIHeroesAdventureCore::Status::Ready){
        std::cout<<"FIXTURE_ERROR: production path data no longer matches the case\n";return 2;
    }
    FAIHeroesCampaignProgress P;P.Total=4;P.Completed=1;
    P.NextFocusId=FName(TEXT("OUTPOST"));
    FAIHeroesCampaignGuideContext C;C.SelectedId=P.NextFocusId;
    C.bTargetDiscovered=true;C.bHasArmy=true;
    C.bNeedsMoreMoves=true;C.MissingMoves=Full.Missing;
    SupplyFacts(C,Full.Cost);
    const auto Card=FAIHeroesCampaignGuide::Build(P,C);
    std::cout<<"PRODUCTION_PATH cost="<<Full.Cost<<" day_budget=4 after_end_day="
             <<Tomorrow.Cost<<" reachable_owned_stop=HQ stop_cost="<<Stop.Cost
             <<" context_cost_fields_available="<<HasCostEvidence<decltype(C)>::value<<"\n";
    const bool OverDayCorrect=!Card.NextStep.Contains(TEXT("закончите день")) &&
        Card.NextStep.Contains(TEXT("Цитадель"));
    if(!OverDayCorrect){
        std::cout<<"ASSERTION_FAILED: over-day route must request a reachable owned stop, not End Day\n";
    }
    const auto Short=AIHeroesAdventureCore::Find(G,"HQ","FOREST",1);
    if(Short.State!=AIHeroesAdventureCore::Status::NeedsMoreMoves || Short.Cost!=2){
        std::cout<<"FIXTURE_ERROR: short path changed\n";return 2;
    }
    P.NextFocusId=FName(TEXT("FOREST"));C.SelectedId=P.NextFocusId;
    C.MissingMoves=Short.Missing;SupplyFacts(C,Short.Cost);
    const auto ShortCard=FAIHeroesCampaignGuide::Build(P,C);
    const bool ShortCorrect=ShortCard.NextStep.Contains(TEXT("закончите день"));
    if(!ShortCorrect)std::cout<<"ASSERTION_FAILED: ordinary remaining-budget shortage still needs End Day\n";
    std::cout<<"GUIDE_CASES over_day="<<OverDayCorrect<<" ordinary_next_day="<<ShortCorrect<<"\n";
    if(OverDayCorrect&&ShortCorrect){
        std::cout<<"GUIDE_OVER_DAY_INSTRUCTION_PASS (portable diagnostic only)\n";return 0;
    }
    return 1;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", action="store_true", help="Compile the archived pre-change guide, not revert production.")
    args = parser.parse_args()
    if not VC.is_file():
        raise RuntimeError("Existing MSVC unavailable; nothing installed")
    selected = list(INPUTS)
    if args.baseline:
        selected[:2] = [str((Path(__file__).parent / ("before_" + Path(p).name)).relative_to(ROOT)).replace("\\", "/") for p in INPUTS[:2]]
    before = [digest(p) for p in selected]
    rules = (ROOT / INPUTS[-1]).read_text(encoding="utf-8-sig")
    roads = re.findall(r'Road\(TEXT\("([A-Z]+)"\), TEXT\("([A-Z]+)"\), (\d+)\);', rules)
    if len(roads) != 5:
        raise RuntimeError("Authored map changed; do not reconstruct old data")
    if "GoalCount = 4" not in (ROOT / INPUTS[2]).read_text(encoding="utf-8-sig"):
        raise RuntimeError("Campaign goal count changed")
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    run = ROOT / "Saved" / "QA" / ("MainGuideDiagnostic_" + stamp)
    run.mkdir(parents=True, exist_ok=False)
    cpp = run / "guide_diagnostic.cpp"
    scenario = SCENARIO.replace("// __ROADS__", "\n".join(
        f'Road("{a}","{b}",{cost});' for a, b, cost in roads))
    cpp.write_text(STUB + body(ROOT / selected[0]) + "\n" + body(ROOT / selected[1]) + scenario, encoding="utf-8")
    exe, obj = run / "guide_diagnostic.exe", run / "guide_diagnostic.obj"
    batch = run / "compile.cmd"
    batch.write_text('@echo off\r\n' + f'call "{VC}" >nul\r\n' +
                     'if errorlevel 1 exit /b 50\r\n' +
                     f'cl.exe /nologo /std:c++17 /EHsc /W4 /WX /utf-8 '
                     f'/I"{ROOT / "Source/AIHeroes/Public"}" "{cpp}" /Fo"{obj}" /Fe"{exe}"\r\n' +
                     'exit /b %errorlevel%\r\n', encoding="ascii", newline="")
    compiled = subprocess.run(["cmd.exe", "/d", "/c", str(batch)], cwd=run, capture_output=True, timeout=100)
    (run / "compile.log").write_bytes(compiled.stdout + compiled.stderr)
    if compiled.returncode:
        print("DIAGNOSTIC_COMPILE_FAILED " + str(run));return 3
    result = subprocess.run([str(exe)], cwd=run, capture_output=True, timeout=20)
    (run / "test.log").write_bytes(result.stdout + result.stderr)
    after = [digest(p) for p in selected]
    if after != before:
        raise RuntimeError("Production source changed during diagnostic; no stable receipt")
    status = "EXPECTED_RED" if result.returncode == 1 else "PASS_PORTABLE_ONLY" if result.returncode == 0 else "FIXTURE_ERROR"
    receipt = {"status": status, "scope": "actual guide header/body with explicit Unreal type shims; actual path kernel",
               "guide_source": "archived pre-change baseline" if args.baseline else "current production",
               "scenario_exit": result.returncode, "native_unreal_run": False,
               "gui_acceptance": False, "production_writes": 0, "save_writes": 0,
               "inputs": before, "shim_source_sha256": hashlib.sha256(cpp.read_bytes()).hexdigest()}
    path = run / "receipt.json"
    path.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(result.stdout.decode("utf-8", errors="replace").strip())
    print("DIAGNOSTIC_RECEIPT=" + str(path))
    return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
