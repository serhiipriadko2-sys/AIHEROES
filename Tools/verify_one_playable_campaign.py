#!/usr/bin/env python3
"""Static integration gate for AIHEROES' first playable campaign.

Checks source-level UI wiring and build-manifest freshness without launching
Unreal, altering the campaign, touching saves, or claiming a gameplay PASS.
Exit codes: 0 = passed selected gates; 1 = source contract failed;
2 = source contract passed but the recorded build snapshot is stale/unknown.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(relative: str) -> str:
    return (ROOT / relative).read_text(encoding="utf-8-sig")


def has(text: str, pattern: str) -> bool:
    return re.search(pattern, text, re.S) is not None


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def source_gate() -> dict[str, bool]:
    controller = read("Source/AIHeroes/Private/AIHeroesPlayerController.cpp")
    explorer = read("Source/AIHeroes/Private/AIHeroesExplorerHUD.cpp")
    layout = read("Source/AIHeroes/Private/AIHeroesExplorerLayout.cpp")
    guide = read("Source/AIHeroes/Private/AIHeroesCampaignGuide.cpp")
    objectives = read("Source/AIHeroes/Private/AIHeroesCampaignObjectives.cpp")
    mode = read("Config/DefaultEngine.ini")
    tests = read("Source/AIHeroes/Private/Tests/AIHeroesPlayableJourneyTests.cpp")
    guidance_tests = read("Source/AIHeroes/Private/Tests/AIHeroesCampaignGuidanceTests.cpp")

    protocol = re.search(
        r'if\s*\(\s*Id\s*==\s*FName\(TEXT\("PROTOCOL"\)\)\s*\)\s*\{(.*?)return Result;',
        guide, re.S,
    )
    model = re.search(
        r'if\s*\(\s*Id\s*==\s*FName\(TEXT\("MODEL_SLOT"\)\)\s*\)\s*\{(.*?)return Result;',
        guide, re.S,
    )
    def next_step(branch):
        match = re.search(r'Result\.NextStep\s*=\s*TEXT\("([^"]*)"\)',
                          branch.group(1)) if branch else None
        return match.group(1) if match else ""
    protocol_step = next_step(protocol)
    model_step = next_step(model)
    return {
        "configured_main_campaign": "GlobalDefaultGameMode=/Script/AIHeroes.AIHeroesGameMode" in mode,
        "canonical_next_objective": all(
            f'Result.NextFocusId = FName(TEXT("{key}"))' in objectives
            or f'Result.NextFocusId=FName(TEXT("{key}"))' in objectives
            for key in ("FOREST", "ORE", "OUTPOST", "RUINS", "PROTOCOL", "MODEL_SLOT", "VICTORY")
        ),
        "guide_reads_canonical_id_not_english_text": "Progress.NextFocusId" in guide
        and "Progress.NextAction" not in guide,
        "keyboard_g_opens_base": has(
            controller, r'BindKey\(\s*EKeys::G\s*,\s*IE_Pressed\s*,\s*this\s*,'
            r'\s*&AAIHeroesPlayerController::ToggleBasePanel\s*\)',
        ),
        "keyboard_y_opens_read_only_lab": has(
            controller, r'BindKey\(\s*EKeys::Y\s*,\s*IE_Pressed\s*,\s*this\s*,'
            r'\s*&AAIHeroesPlayerController::OpenProtocolLab\s*\)',
        ),
        "click_controls_match_keyboard": has(
            controller, r'case 203:\s*OpenProtocolLab\(\);\s*break;'
        ) and has(
            controller, r'case 202:\s*ShowCampaignObjectives\(\);\s*break;'
        ) and has(
            controller, r'if\s*\(\s*ActionIndex\s*==\s*FAIHeroesBasePanelRules::ToggleAction\s*\)'
            r'\s*\{\s*Game->ToggleBasePanel\(\);\s*return;\s*\}',
        ),
        "buttons_label_real_actions": has(
            explorer, r'Button\(\s*400\s*,\s*TEXT\("Постройки \[G\]"\)'
        ) and has(
            explorer, r'Button\(\s*203\s*,\s*TEXT\("Тактики \[Y\]"\)'
        ),
        "protocol_goal_points_to_purchase_not_lab": "Постройках" in protocol_step
        and "[G]" in protocol_step and "[Y]" not in protocol_step,
        "model_slot_goal_points_to_base": "[G]" in model_step
        and "слот" in model_step.lower(),
        "goal_ribbon_is_read_only_shortcut": has(
            layout, r'if\(\s*!Open\s*&&\s*F\.Mission\.Contains\(P\)\s*\)\s*return 202;'
        ),
        "guide_filters_hidden_goal": "G.IsNodeDiscovered(Progress.NextFocusId)" in explorer,
        "guarded_order_uses_shared_display_eligibility": has(
            explorer, r'OfferReady\s*=\s*FAIHeroesCampaignGuide::CanOfferOrder\('
            r'\s*Ready,\s*GuideContext.bHasArmy,\s*Path.bBattleAtDestination\s*\)',
        ) and has(explorer, r'Button\(0,CTA,true,OfferReady\)'),
        "guide_only_requires_army_for_guarded_objective": has(
            guide, r'if\(\s*!Context.bHasArmy\s*&&\s*Context.bTargetGuarded\s*\)',
        ),
        "named_stop_is_independently_reachable": has(
            explorer, r'StopPath\s*=\s*G.GetTravelPathTo\(StopId\);\s*'
            r'if\(StopPath.IsReachableThisTurn\(\)\)\s*'
            r'GuideContext.IntermediateOwnedSite=StopId;',
        ),
        "order_and_waypoint_regressions_registered": all(
            name in guidance_tests for name in (
                "EmptyArmyGuidanceRespectsBattleRequirement", "OrderEligibilityTruthTable",
                "ExhaustedMovesRestOnlyForStagingLeg", "WaypointQueryDoesNotSelectOrReveal",
            )
        ),
        "regression_checks_registered": all(
            name in tests for name in (
                "CanonicalGoalSequence", "ContextualInstruction",
                "FogAndArmySafety", "UpgradeMilestones", "GoalsRibbonTapAllScreens",
            )
        ),
    }


def build_gate() -> dict:
    manifest_path = ROOT / "Docs/AIHEROES_BUILD_LATEST.json"
    if not manifest_path.is_file():
        return {"fresh": False, "reason": "NO_BUILD_MANIFEST"}
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
        inputs = manifest["inputs"]
        seen: set[str] = set()
        drift: list[str] = []
        for item in inputs:
            rel = item["path"].replace("\\", "/")
            if rel in seen or rel.startswith("/") or ".." in Path(rel).parts:
                drift.append("UNSAFE_OR_DUPLICATE:" + rel)
                continue
            seen.add(rel)
            target = ROOT / rel
            if not target.is_file():
                drift.append("MISSING:" + rel)
            elif target.stat().st_size != item["bytes"] or sha256(target) != item["sha256"].lower():
                drift.append("CHANGED:" + rel)
        expected = set()
        for ext in ("*.cpp", "*.h"):
            expected.update(p.relative_to(ROOT).as_posix() for p in (ROOT / "Source").rglob(ext))
        expected.update(p.relative_to(ROOT).as_posix() for p in (ROOT / "Config").rglob("*.ini"))
        expected.add("AIHeroes.uproject")
        unlisted = sorted(expected - seen)
        binary = manifest.get("binary", {})
        binary_path = ROOT / str(binary.get("path", ""))
        binary_match = bool(binary.get("sha256")) and binary_path.is_file() and (
            sha256(binary_path) == binary["sha256"].lower()
        )
        return {
            "fresh": not drift and not unlisted and binary_match,
            "manifest_inputs": len(inputs),
            "changed_or_missing": drift[:25],
            "changed_or_missing_count": len(drift),
            "unlisted_source": unlisted[:25],
            "unlisted_source_count": len(unlisted),
            "editor_binary_matches_manifest": binary_match,
            "note": "Editor compile freshness only; no cook, package, native tests or human playthrough.",
        }
    except (OSError, KeyError, TypeError, ValueError) as exc:
        return {"fresh": False, "reason": "INVALID_MANIFEST", "detail": str(exc)[:200]}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--static-only", action="store_true", help="Skip source/build SHA freshness gate.")
    args = parser.parse_args()
    try:
        checks = source_gate()
        static_ok = all(checks.values())
    except (OSError, UnicodeError) as exc:
        checks = {"source_files_readable": False}
        static_ok = False
        error = str(exc)
    else:
        error = ""
    build = {"fresh": None, "reason": "SKIPPED"} if args.static_only else build_gate()
    code = 1 if not static_ok else 0 if args.static_only or build["fresh"] else 2
    report = {
        "schema": "aiheroes.playable-loop-static-gate.v1",
        "status": "STATIC_FAIL" if code == 1 else "STATIC_PASS" if args.static_only
                  else "STATIC_PASS_BUILD_STALE" if code == 2 else "STATIC_PASS_BUILD_MATCH",
        "source_checks": checks,
        "source_error": error,
        "build": build,
        "native_automation_executed": False,
        "gameplay_playthrough_executed": False,
        "filesystem_mutations_by_verifier": 0,
    }
    print(json.dumps(report, ensure_ascii=False, indent=2))
    return code


if __name__ == "__main__":
    raise SystemExit(main())
