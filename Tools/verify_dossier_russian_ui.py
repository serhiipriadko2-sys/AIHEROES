#!/usr/bin/env python3
"""Read-only, source-only QA of the Russian dossier HUD.

Verifies source wiring and evidence stability. A green report does NOT mean
the Unreal C++ module was compiled or any rendered pixels were inspected.
"""
from __future__ import annotations

import hashlib
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PATHS = {
    "rules_h": "Source/AIHeroes/Public/AIHeroesAdvisorDossier.h",
    "rules_cpp": "Source/AIHeroes/Private/AIHeroesAdvisorDossier.cpp",
    "hud": "Source/AIHeroes/Private/AIHeroesAdvisorDossierHUD.cpp",
    "tests": "Source/AIHeroes/Private/Tests/AIHeroesAdvisorDossierTests.cpp",
}


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    text: dict[str, str] = {}
    before: dict[str, str] = {}
    errors: list[str] = []
    for key, rel in PATHS.items():
        file = ROOT / rel
        try:
            before[key] = sha256(file)
            text[key] = file.read_text(encoding="utf-8-sig")
        except (OSError, UnicodeError) as exc:
            errors.append(f"{rel}: {type(exc).__name__}")
    checks: dict[str, bool] = {}
    if not errors:
        hud = text["hud"]
        rules = text["rules_cpp"]
        header = text["rules_h"]
        tests = text["tests"]
        checks = {
            "localized_title_from_existing_report_category": (
                "DisplayTitle(Dossier)" in hud
                and all(x in rules for x in (
                    'TEXT("ЦЕЛИ КАМПАНИИ")',
                    'TEXT("СОВЕТ ПО АРМИИ")',
                    'TEXT("СРАВНЕНИЕ ТАКТИК")',
                    "return Dossier.Title;",
                ))
            ),
            "stale_and_fresh_are_distinct": (
                "FreshnessLabel(bFresh)" in hud
                and 'TEXT("ДАННЫЕ УСТАРЕЛИ — ОБНОВИТЕ")' in rules
                and 'TEXT("ДАННЫЕ АКТУАЛЬНЫ")' in rules
            ),
            "four_russian_buttons_match_existing_actions": (
                "ActionLabel(" in hud
                and all(f"case {action}: return TEXT(" in rules for action in (
                    "PreviousAction", "NextAction", "RefreshAction", "CloseAction"
                ))
            ),
            "actual_font_measurement_includes_cyrillic": (
                "GetTextSize(FString::Chr(Cyrillic[I])" in hud
                and "ДЕЁЖ" in hud and "деёж" in hud
                and "DossierGlyphWidth" in hud
            ),
            "headings_use_pixel_fitting": (
                "FAIHeroesHUDTextLayout::FitLine" in hud
                and 'L.TextWidth, Scale * 1.15f' in hud
            ),
            "full_report_retained_as_paginated_content": (
                "FAIHeroesAdvisorDossierRules::Wrap(Dossier.Paragraphs, Columns)" in hud
                and "DrawText(Lines[Index]" in hud
                and "MaxCharacters" in header
            ),
            "no_change_to_dossier_input_lock": (
                "FAIHeroesAdvisorDossierRules::HitTest" in rules
                and "return ConsumeAction" in rules
                and "Rules->IsAdvisorDossierVisible()" in hud
            ),
            "russian_and_long_token_regression_registered": (
                "AIHeroes.Dossier.RussianCopyAndUnicode" in tests
                and "Cyrillic report content survives wrapping" in tests
                and 'FString::Join(Wrapped, TEXT(""))' in tests
            ),
            "unknown_source_title_kept_verbatim": "return Dossier.Title;" in rules,
            "display_does_not_issue_player_commands": (
                "ConfirmBasePurchase(" not in hud
                and "InteractWithNode(" not in hud
                and "EndTurn(" not in hud
            ),
        }

    checks["all_sources_stable_during_qa"] = not errors and all(
        sha256(ROOT / rel) == before[key] for key, rel in PATHS.items()
    )
    diff = subprocess.run(
        ["git", "diff", "--check", "--", *PATHS.values()],
        cwd=ROOT, capture_output=True, text=True, check=False,
    )
    checks["git_diff_has_no_whitespace_errors"] = diff.returncode == 0
    ok = not errors and all(checks.values())
    result = {
        "schema": "aiheroes.dossier-russian-source-qa.v1",
        "status": "STATIC_PASS_NATIVE_NOT_RUN" if ok else "STATIC_FAIL",
        "checks": checks,
        "errors": errors,
        "file_sha256": before,
        "native_unreal_build": "NOT_RUN",
        "unreal_automation": "NOT_RUN",
        "screenshot_review": "NOT_RUN",
        "saved_game_writes": 0,
        "verifier_writes": 0,
    }
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
