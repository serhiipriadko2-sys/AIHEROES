#!/usr/bin/env python3
"""Read-only source integrity gate for the playable campaign's Russian base UI.

This checks wiring and file stability; it does not compile C++, execute
Unreal automation, render a screenshot, change saves or authorize purchases.
"""
from __future__ import annotations

import hashlib
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FILES = {
    "copy_header": "Source/AIHeroes/Public/AIHeroesBasePanelCopy.h",
    "copy_source": "Source/AIHeroes/Private/AIHeroesBasePanelCopy.cpp",
    "base_hud": "Source/AIHeroes/Private/AIHeroesBasePanelHUD.cpp",
    "copy_tests": "Source/AIHeroes/Private/Tests/AIHeroesBasePanelCopyTests.cpp",
    "base_game_mode": "Source/AIHeroes/Private/AIHeroesBasePanelGameMode.cpp",
    "base_rules": "Source/AIHeroes/Private/AIHeroesBasePanel.cpp",
}


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def main() -> int:
    hashes_before: dict[str, str] = {}
    sources: dict[str, str] = {}
    errors: list[str] = []
    for name, relative in FILES.items():
        path = ROOT / relative
        try:
            hashes_before[name] = digest(path)
            sources[name] = path.read_text(encoding="utf-8-sig")
        except (OSError, UnicodeError) as exc:
            errors.append(f"{relative}: {type(exc).__name__}")

    checks: dict[str, bool] = {}
    if not errors:
        h = sources["copy_header"]
        c = sources["copy_source"]
        ui = sources["base_hud"]
        tests = sources["copy_tests"]
        mode = sources["base_game_mode"]
        rules = sources["base_rules"]

        checks = {
            "main_ui_draws_localized_title_price_and_status": all(
                f"FAIHeroesBasePanelCopy::{method}(" in ui
                for method in ("Title", "Price", "Status", "Detail", "Message")
            ),
            "all_five_purchase_categories_have_display_copy": all(
                f"case EAIHeroesBasePurchase::{kind}:" in c
                for kind in ("Headquarters", "Militia", "Scouts", "Protocol", "ModelSlot")
            ),
            "localization_never_recalculates_purchase_amounts": (
                "Gold - Selected.Gold" not in c
                and "Wood - Selected.Wood" not in c
                and "Ore - Selected.Ore" not in c
                and "Data - Selected.Data" not in c
                and "Display = Raw;" in c
                and 'Display.ReplaceInline(TEXT("AFTER PURCHASE: ")' in c
            ),
            "copy_message_only_consumes_backend_text_and_selected_offer": (
                "const FString& Raw,\n    const FAIHeroesBasePanelOffer& Selected)" in c
                and "const FAIHeroesBasePanelOffer& Selected);" in h
                and "Rules.GetBasePanelMessage(), SelectedOffer)" in ui
            ),
            "unknown_engine_errors_are_not_hidden": "return Raw;" in c,
            "backend_uses_actual_quote_and_confirms_once": (
                "CanConfirmBasePurchase()" in mode
                and "PendingBaseFingerprint == CampaignFingerprint()" in mode
                and "TGuardValue<bool> ReleasePanelLock" in mode
                and "PendingBaseFingerprint.Empty();" in mode
            ),
            "backend_prices_and_purchase_rules_remain_separate": (
                "FAIHeroesBasePanelRules::Offers" in rules
                and "FAIHeroesBasePanelCopy" not in rules
                and "UpgradeProtocol();" in mode
                and "UpgradeModelSlot();" in mode
            ),
            "automatic_unreal_test_cases_registered": (
                "AIHeroes.UI.BasePanelCopy.OfferStates" in tests
                and "AIHeroes.UI.BasePanelCopy.SafetyAndMessages" in tests
            ),
            "tests_assert_backend_preview_amounts_not_ui_arithmetic": (
                "Preview uses exactly the GameMode-provided amounts" in tests
                and "После покупки: 1 зол. / 2 дер. / 3 руд. / 4 дан." in tests
            ),
            "status_and_key_actions_are_russian": all(
                phrase in ui for phrase in ("ЗАКРЫТЬ / ESC", "КУПИТЬ / ENTER", "ЦИТАДЕЛЬ")
            ),
        }

    stable = not errors and all(
        digest(ROOT / relative) == hashes_before[name]
        for name, relative in FILES.items()
    )
    checks["source_hash_stable_during_verification"] = stable

    # Report build freshness separately from the static source contract.
    recorded = ROOT / "Docs/AIHEROES_BUILD_LATEST.json"
    build_matched = False
    if recorded.is_file() and not errors:
        try:
            manifest = json.loads(recorded.read_text(encoding="utf-8-sig"))
            inputs = {x["path"].replace("\\", "/"): x for x in manifest["inputs"]}
            build_matched = all(
                inputs.get(FILES[k], {}).get("sha256") == hashes_before[k]
                for k in ("copy_header", "copy_source", "base_hud", "copy_tests")
            )
        except (OSError, TypeError, KeyError, ValueError):
            pass

    diff = subprocess.run(
        ["git", "diff", "--check", "--", FILES["base_hud"]],
        cwd=ROOT, capture_output=True, text=True, check=False,
    )
    checks["hud_diff_has_no_whitespace_errors"] = diff.returncode == 0
    ok = not errors and all(checks.values())
    report = {
        "schema": "aiheroes.base-ui-static-qa.v1",
        "status": ("STATIC_FAIL" if not ok else
                   "STATIC_PASS_BUILD_MATCH" if build_matched
                   else "STATIC_PASS_BUILD_STALE"),
        "checks": checks,
        "errors": errors,
        "source_sha256": hashes_before,
        "build_manifest_matches_feature_sources": build_matched,
        "native_unreal_compilation": "NOT_RUN",
        "native_unreal_tests": "NOT_RUN",
        "window_render_qa": "NOT_RUN",
        "mutation_by_verifier": 0,
    }
    print(json.dumps(report, indent=2, ensure_ascii=False))
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
