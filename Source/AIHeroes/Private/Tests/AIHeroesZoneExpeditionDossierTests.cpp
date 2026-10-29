#include "AIHeroesZoneExpeditionAdvisor.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesZoneExpeditionRules.h"
#include "Misc/AutomationTest.h"
#include "UObject/Class.h"

namespace
{
FAIHeroesZoneExpeditionState CampaignFor(uint32 Seed=20261029u, int32 Branches=3)
{
    FAIHeroesZoneExpeditionState S;
    FString Error;
    FAIHeroesZoneExpeditionRules::Create(Seed, Branches, S, Error);
    return S;
}

const FAIHeroesZoneDossierOption* Find(
    const FAIHeroesZoneExpeditionDossier& D, FName Target)
{
    return D.Options.FindByPredicate(
        [Target](const FAIHeroesZoneDossierOption& O)
        { return O.Site == Target; });
}

bool AllOwnedFieldsEqual(const FAIHeroesZoneExpeditionState& A,
                          const FAIHeroesZoneExpeditionState& B)
{
    return FAIHeroesZoneExpeditionState::StaticStruct()
        ->CompareScriptStruct(&A, &B, 0);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesDossierKnownOnly,
    "AIHeroes.ZoneExpeditionDossier.ThreeKnownOptionsNoMutation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesDossierKnownOnly::RunTest(const FString&)
{
    auto S = CampaignFor();
    const auto Old = S;
    FString Error;
    FAIHeroesAdventureGraph G;
    if (!TestTrue(TEXT("Visible canonical view exists"),
        FAIHeroesZoneExpeditionRules::PlayerView(S, G, Error)))
        return false;
    const auto D = FAIHeroesZoneExpeditionAdvisor::CompareKnown(S);
    TestTrue(TEXT("Read-only dossier validated"), D.bValid);
    TestEqual(TEXT("Only three initial unclaimed sites"), D.ConsideredKnownTargets, 3);
    TestEqual(TEXT("Exactly three alternatives displayed"), D.Options.Num(), 3);
    TestEqual(TEXT("Visible revision matches source"), D.SourceRevision, G.Revision);
    if (!TestTrue(TEXT("First option is known"), D.Options.Num() == 3))
        return false;
    TestEqual(TEXT("Baseline known safe supply route outranks combat"),
        D.Options[0].Site, FName(TEXT("SUPPLY")));
    for (const auto& O : D.Options)
    {
        TestTrue(TEXT("Every candidate was independently visible"),
            G.Nodes.ContainsByPredicate(
                [&O](const FAIHeroesAdventureNode& N)
                { return N.Id == O.Site; }));
        TestFalse(TEXT("No hidden objective appears"),
            O.Site == FName(TEXT("OUTPOST")) ||
            O.Site == FName(TEXT("RUINS")) ||
            O.Site == FName(TEXT("PASS")));
        const auto Route = FAIHeroesAdventurePath::Find(
            G, S.HeroSite, O.Site, S.MovesLeft);
        TestEqual(TEXT("Exact authoritative movement cost"),
            O.TravelCost, Route.TotalCost);
        TestEqual(TEXT("Destination guard matches known route"),
            O.bGuarded, Route.bBattleAtDestination);
        TestTrue(TEXT("Never report fabricated casualty numbers"),
            !O.Assessment.Contains(TEXT("%")) &&
            !O.Assessment.Contains(TEXT("WIN FORECAST")));
    }
    TestTrue(TEXT("Query never mutates whole UStruct"),
        AllOwnedFieldsEqual(S, Old));
    const auto Again = FAIHeroesZoneExpeditionAdvisor::CompareKnown(S);
    for (int32 I = 0; I < D.Options.Num(); ++I)
    {
        TestEqual(TEXT("Stable ordering on same snapshot"),
            D.Options[I].Site, Again.Options[I].Site);
        TestEqual(TEXT("Stable score on same snapshot"),
            D.Options[I].RankScore, Again.Options[I].RankScore);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesDossierConstraints,
    "AIHeroes.ZoneExpeditionDossier.GuardTroopsAndMovement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesDossierConstraints::RunTest(const FString&)
{
    auto S = CampaignFor();
    auto D = FAIHeroesZoneExpeditionAdvisor::CompareKnown(S);
    const auto* Forest = Find(D, FName(TEXT("FOREST")));
    if (!TestTrue(TEXT("Known guarded FOREST compared"), Forest != nullptr))
        return false;
    TestTrue(TEXT("Combat remains outcome UNKNOWN"),
        Forest->bGuarded &&
        Forest->Assessment.Contains(TEXT("OUTCOME UNKNOWN")));
    TestTrue(TEXT("Battle needs a human confirmation"),
        Forest->bReachableNow);

    S.PlayerArmy.Empty();
    D = FAIHeroesZoneExpeditionAdvisor::CompareKnown(S);
    Forest = Find(D, FName(TEXT("FOREST")));
    TestTrue(TEXT("No-army guarded option is blocked"),
        Forest && Forest->Readiness == EAIHeroesDossierReadiness::NoLivingArmy &&
        !Forest->bReachableNow);
    const auto* Supply = Find(D, FName(TEXT("SUPPLY")));
    TestTrue(TEXT("Ungarrisoned unguarded supply stays navigable"),
        Supply && Supply->Readiness == EAIHeroesDossierReadiness::ReadyNoGuard &&
        Supply->bReachableNow);

    S = CampaignFor();
    S.MovesLeft = 0;
    D = FAIHeroesZoneExpeditionAdvisor::CompareKnown(S);
    TestTrue(TEXT("Zero moves still permits read-only future comparison"),
        D.bValid && !D.Options.IsEmpty());
    for (const auto& O : D.Options)
    {
        TestEqual(TEXT("All zero-move options require next day"),
            O.Readiness, EAIHeroesDossierReadiness::InsufficientMovement);
        TestTrue(TEXT("Movement deficit explicit and not an order"),
            O.MissingMoves > 0 && !O.bReachableNow);
    }
    S = CampaignFor();
    S.PlayerArmy[0].TotalHealth = 12;
    S.PlayerArmy[1].TotalHealth = 8;
    D = FAIHeroesZoneExpeditionAdvisor::CompareKnown(S);
    Forest = Find(D, FName(TEXT("FOREST")));
    TestTrue(TEXT("Low HP only triggers capacity caution, not defeat claim"),
        Forest && Forest->Readiness == EAIHeroesDossierReadiness::ThinReserves &&
        Forest->Assessment.Contains(TEXT("OUTCOME UNKNOWN")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesDossierSeedSweep,
    "AIHeroes.ZoneExpeditionDossier.SeedSweep640AndStableRanking",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesDossierSeedSweep::RunTest(const FString&)
{
    int32 Valid = 0;
    FString Error;
    for (uint32 Seed = 0; Seed < 128; ++Seed)
        for (int32 Branches = 0; Branches <= 4; ++Branches)
        {
            auto S = CampaignFor(Seed, Branches);
            const auto Before = S;
            FAIHeroesAdventureGraph G;
            if (!FAIHeroesZoneExpeditionRules::PlayerView(S, G, Error))
                continue;
            const auto D = FAIHeroesZoneExpeditionAdvisor::CompareKnown(S);
            if (!D.bValid || D.Options.IsEmpty() || D.Options.Num() > 3 ||
                D.SourceRevision != G.Revision ||
                !AllOwnedFieldsEqual(S, Before))
                continue;
            bool bOkay = true;
            for (int32 I = 0; I < D.Options.Num(); ++I)
            {
                const auto& O = D.Options[I];
                const auto Path = FAIHeroesAdventurePath::Find(
                    G, S.HeroSite, O.Site, S.MovesLeft);
                if (!G.Nodes.ContainsByPredicate(
                    [&O](const FAIHeroesAdventureNode& N)
                    { return N.Id == O.Site; }) ||
                    O.TravelCost != Path.TotalCost ||
                    Path.Status != EAIHeroesAdventurePathStatus::Ready)
                    bOkay = false;
                if (I > 0 && D.Options[I-1].RankScore < O.RankScore)
                    bOkay = false;
            }
            if (bOkay) ++Valid;
        }
    TestEqual(TEXT("640 generated worlds produce legal non-leaking rankings"),
        Valid, 640);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesDossierAfterBattle,
    "AIHeroes.ZoneExpeditionDossier.InvalidSnapshotAndNewDiscovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesDossierAfterBattle::RunTest(const FString&)
{
    auto S = CampaignFor();
    const auto Baseline = FAIHeroesZoneExpeditionAdvisor::CompareKnown(S);
    TestTrue(TEXT("Initial hidden checkpoint not exposed"),
        Find(Baseline, FName(TEXT("PASS"))) == nullptr);
    S.MapRevision ^= 1u;
    const auto Tampered = FAIHeroesZoneExpeditionAdvisor::CompareKnown(S);
    TestTrue(TEXT("Tampered map fails closed"),
        !Tampered.bValid && Tampered.Options.IsEmpty() &&
        Tampered.SourceRevision == 0 && Tampered.ConsideredKnownTargets == 0);
    TestFalse(TEXT("Error never reveals hidden checkpoint"),
        Tampered.Summary.Contains(TEXT("PASS")));
    S = CampaignFor();
    FAIHeroesZoneExpeditionResult Result;
    if (!TestTrue(TEXT("Human separately authorizes FOREST battle"),
        FAIHeroesZoneExpeditionRules::ConfirmPlayerDestination(
            S, FName(TEXT("FOREST")), Result)))
        return false;
    const auto Before = S;
    const auto After = FAIHeroesZoneExpeditionAdvisor::CompareKnown(S);
    TestTrue(TEXT("New player-view epoch after genuine reconnaissance"),
        After.bValid && After.SourceRevision != Baseline.SourceRevision);
    TestTrue(TEXT("One new visible objective is a candidate"),
        After.ConsideredKnownTargets >= Baseline.ConsideredKnownTargets);
    TestTrue(TEXT("Advice cannot execute a second battle"),
        AllOwnedFieldsEqual(S, Before));
    TestTrue(TEXT("Visible guard cannot be automatically conquered"),
        !FAIHeroesZoneExpeditionRules::IsCleared(S, FName(TEXT("PASS"))));
    return true;
}
#endif
