#include "AIHeroesZoneExpeditionRules.h"
#include "AIHeroesZoneExpeditionStore.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesCampaignSave.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"

namespace
{
FAIHeroesZoneExpeditionState Sample(uint32 Seed = 20261029u, int32 Branches = 3)
{
    FAIHeroesZoneExpeditionState Out;
    FString Error;
    FAIHeroesZoneExpeditionRules::Create(Seed, Branches, Out, Error);
    return Out;
}

int32 Health(const FAIHeroesZoneExpeditionState& S)
{
    int32 Total = 0;
    for (const auto& Unit : S.PlayerArmy) Total += Unit.TotalHealth;
    return Total;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FZoneExpeditionStartTest,
    "AIHeroes.ZoneExpedition.SeededCampaignAndHiddenTopology",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FZoneExpeditionStartTest::RunTest(const FString&)
{
    FString Error;
    FAIHeroesZoneExpeditionState S;
    if (!TestTrue(TEXT("Original 11-site campaign state creates"),
        FAIHeroesZoneExpeditionRules::Create(20261029u, 4, S, Error))) return false;
    TestEqual(TEXT("New state schema v2"), S.Schema, 2);
    TestEqual(TEXT("Starting discovered sites"), FAIHeroesZoneExpeditionRules::DiscoveredCount(S), 4);
    TestEqual(TEXT("HQ is current hero position"), S.HeroSite, FName(TEXT("HQ")));
    TestTrue(TEXT("Initial data validates"), FAIHeroesZoneExpeditionRules::Validate(S, Error));
    TestFalse(TEXT("PASS is hidden"), FAIHeroesZoneExpeditionRules::IsDiscovered(
        S, FName(TEXT("PASS"))));
    TestFalse(TEXT("Objective OUTPOST is hidden"), FAIHeroesZoneExpeditionRules::IsDiscovered(
        S, FName(TEXT("OUTPOST"))));

    FAIHeroesAdventureGraph View;
    if (!TestTrue(TEXT("Player-view graph builds"),
        FAIHeroesZoneExpeditionRules::PlayerView(S, View, Error))) return false;
    TestEqual(TEXT("Visible graph includes four nodes, not 11"), View.Nodes.Num(), 4);
    for (const auto& Edge : View.Links)
    {
        TestFalse(TEXT("No hidden checkpoint name in visible links"),
            Edge.From == FName(TEXT("PASS")) || Edge.To == FName(TEXT("PASS")));
    }
    const auto Hidden = FAIHeroesZoneExpeditionRules::Preview(S, TEXT("RUINS"));
    TestFalse(TEXT("Hidden goal unavailable even with template oracle"), Hidden.IsReachableThisTurn());
    const auto First = FAIHeroesZoneExpeditionRules::Preview(S, TEXT("FOREST"));
    TestTrue(TEXT("First guarded resource reachable by known road"), First.IsReachableThisTurn());
    TestTrue(TEXT("Guard is a player-confirmed battle proposal"), First.bBattleAtDestination);
    TestTrue(TEXT("Preview never captures a node"), !FAIHeroesZoneExpeditionRules::IsCleared(S, TEXT("FOREST")));
    TestEqual(TEXT("Preview does not spend moves"), S.MovesLeft, 8);

    FAIHeroesZoneExpeditionState Repeat = Sample(20261029u, 4);
    TestEqual(TEXT("Same seed and branches yield same topology revision"), Repeat.MapRevision, S.MapRevision);
    TestTrue(TEXT("Current five-site SaveGame schema stays 1"),
        UAIHeroesCampaignSave::CurrentSchema == 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FZoneExpeditionAuthorityTest,
    "AIHeroes.ZoneExpedition.PlayerConfirmedCombatAndReconnaissance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FZoneExpeditionAuthorityTest::RunTest(const FString&)
{
    FAIHeroesZoneExpeditionState S = Sample();
    FString Error;
    FAIHeroesZoneExpeditionResult Action;
    const int32 BeforeHp = Health(S);
    const int32 BeforeMoves = S.MovesLeft;
    const auto Path = FAIHeroesZoneExpeditionRules::Preview(S, TEXT("FOREST"));
    TestFalse(TEXT("Cannot execute hidden PASS before reconnaissance"),
        FAIHeroesZoneExpeditionRules::ConfirmPlayerDestination(S, TEXT("PASS"), Action));
    TestEqual(TEXT("Rejection does not alter movement"), S.MovesLeft, BeforeMoves);
    TestEqual(TEXT("Rejection does not change troops"), Health(S), BeforeHp);

    if (!TestTrue(TEXT("Only explicit player confirm starts encounter"),
        FAIHeroesZoneExpeditionRules::ConfirmPlayerDestination(S, TEXT("FOREST"), Action))) return false;
    TestTrue(TEXT("A real Grid V2 encounter was required"), Action.bBattle);
    TestTrue(TEXT("Starting force wins deterministic first encounter"), Action.bVictory);
    TestTrue(TEXT("Forest ownership applied once"), FAIHeroesZoneExpeditionRules::IsCleared(S, TEXT("FOREST")));
    TestTrue(TEXT("Checkpoint discovered after arrival"), FAIHeroesZoneExpeditionRules::IsDiscovered(S, TEXT("PASS")));
    TestEqual(TEXT("Movement deducted exactly once"), S.MovesLeft, BeforeMoves - Path.TotalCost);
    TestEqual(TEXT("Battle earned 2 data"), S.DataFragments, 2);
    TestTrue(TEXT("Battle left at most starter HP"), Health(S) <= BeforeHp);
    TestTrue(TEXT("Post-battle state validates"), FAIHeroesZoneExpeditionRules::Validate(S, Error));

    const auto ToPass = FAIHeroesZoneExpeditionRules::Preview(S, TEXT("PASS"));
    TestFalse(TEXT("Cannot cross a living checkpoint to OUTPOST"),
        FAIHeroesZoneExpeditionRules::Preview(S, TEXT("OUTPOST")).IsReachableThisTurn());
    TestTrue(TEXT("Checkpoint revealed, but still guarded"),
        FAIHeroesZoneExpeditionRules::IsDiscovered(S, TEXT("PASS")));
    TestFalse(TEXT("Checkpoint not secretly captured by discovering it"),
        FAIHeroesZoneExpeditionRules::IsCleared(S, TEXT("PASS")));

    // No hidden goal or shopping action is issued by a preview.
    const uint32 BeforeClear = S.ClearedMask;
    const uint32 BeforeKnown = S.DiscoveredMask;
    (void)ToPass;
    TestEqual(TEXT("Read-only preview leaves cleared mask intact"), S.ClearedMask, BeforeClear);
    TestEqual(TEXT("Read-only preview leaves known mask intact"), S.DiscoveredMask, BeforeKnown);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FZoneExpeditionTamperTest,
    "AIHeroes.ZoneExpedition.TamperedSaveAndGuardBypassFailClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FZoneExpeditionTamperTest::RunTest(const FString&)
{
    const FAIHeroesZoneExpeditionState Original = Sample();
    FString Error;
    auto S = Original;
    ++S.MapRevision;
    TestFalse(TEXT("Untrusted changed map revision rejected"),
        FAIHeroesZoneExpeditionRules::Validate(S, Error));
    S = Original;
    S.DiscoveredMask |= (1u << 15);
    TestFalse(TEXT("Unknown zone bit rejected"),
        FAIHeroesZoneExpeditionRules::Validate(S, Error));
    S = Original;
    S.DiscoveredMask |= (1u << 5);
    TestFalse(TEXT("Map oracle cannot mark an objective discovered early"),
        FAIHeroesZoneExpeditionRules::Validate(S, Error));
    S = Original;
    S.ClearedMask |= (1u << 4); // forged checkpoint victory
    S.DiscoveredMask |= (1u << 4) | (1u << 5) | (1u << 6);
    TestFalse(TEXT("Forged checkpoint bypass rejected"),
        FAIHeroesZoneExpeditionRules::Validate(S, Error));
    S = Original;
    S.HeroSite = FName(TEXT("RUINS"));
    TestFalse(TEXT("Hero cannot be forged into a hidden objective"),
        FAIHeroesZoneExpeditionRules::Validate(S, Error));
    S = Original;
    S.Day = 366;
    TestFalse(TEXT("Exceeded max day rejected"), FAIHeroesZoneExpeditionRules::Validate(S, Error));
    S = Original;
    S.PlayerArmy[0].Attack = 999;
    TestFalse(TEXT("Forged friendly combat stats rejected"),
        FAIHeroesZoneExpeditionRules::Validate(S, Error));

    S = Original;
    FAIHeroesZoneExpeditionResult Result;
    const auto Before = S.DiscoveredMask;
    TestFalse(TEXT("Direct hidden objective order blocked"),
        FAIHeroesZoneExpeditionRules::ConfirmPlayerDestination(S, TEXT("OUTPOST"), Result));
    TestEqual(TEXT("Rejected action never exposes a zone"), S.DiscoveredMask, Before);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FZoneExpeditionEconomyTest,
    "AIHeroes.ZoneExpedition.DaysRecruitmentAndProtocolCosts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FZoneExpeditionEconomyTest::RunTest(const FString&)
{
    auto S = Sample();
    FString Error;
    const int32 OriginalHp = Health(S);
    TestTrue(TEXT("At HQ player can recruit five Militia"),
        FAIHeroesZoneExpeditionRules::RecruitAtHeadquarters(S, Error));
    TestEqual(TEXT("Recruitment charges Gold once"), S.Gold, 190);
    TestEqual(TEXT("Recruitment charges Wood once"), S.Wood, 30);
    TestEqual(TEXT("Recruitment adds exact HP"), Health(S), OriginalHp + 60);
    TestFalse(TEXT("Protocol cannot unlock without earned DATA"),
        FAIHeroesZoneExpeditionRules::UpgradeProtocol(S, Error));
    TestEqual(TEXT("No free protocol"), S.ProtocolTier, 0);
    TestTrue(TEXT("Player explicitly ends a day"),
        FAIHeroesZoneExpeditionRules::EndPlayerDay(S, Error));
    TestEqual(TEXT("Day changed exactly once"), S.Day, 2);
    TestEqual(TEXT("Movement restored"), S.MovesLeft, 8);
    TestEqual(TEXT("Base gold income paid"), S.Gold, 230);
    S.Day = 365;
    const int32 AtLimit = S.Gold;
    TestFalse(TEXT("End day is blocked at bounded max"), 
        FAIHeroesZoneExpeditionRules::EndPlayerDay(S, Error));
    TestEqual(TEXT("Failed day change spends no Gold"), S.Gold, AtLimit);
    S.Day = 2;
    S.Gold = 1000000;
    TestFalse(TEXT("Income overflow rejected atomically"),
        FAIHeroesZoneExpeditionRules::EndPlayerDay(S, Error));
    TestEqual(TEXT("Rejected income keeps old day"), S.Day, 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FZoneExpeditionStoreTest,
    "AIHeroes.ZoneExpedition.IsolatedAandBSaveReadbackAndRecovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FZoneExpeditionStoreTest::RunTest(const FString&)
{
    const FString Prefix = FString::Printf(TEXT("AIHEROES_ZoneExpedition_Test_%s"),
        *FGuid::NewGuid().ToString(EGuidFormats::Digits));
    const FString A = FAIHeroesZoneExpeditionStore::SlotA(Prefix);
    const FString B = FAIHeroesZoneExpeditionStore::SlotB(Prefix);
    auto Clean = [&]()
    {
        UGameplayStatics::DeleteGameInSlot(A, 0);
        UGameplayStatics::DeleteGameInSlot(B, 0);
    };
    Clean();
    FAIHeroesZoneExpeditionState S = Sample();
    FString Message;
    TestFalse(TEXT("Never allow old campaign's slot prefix"),
        FAIHeroesZoneExpeditionStore::Save(S, TEXT("AIHEROES_Campaign_Resilient_v1"), Message));
    TestFalse(TEXT("Reject arbitrary file/path prefix"),
        FAIHeroesZoneExpeditionStore::Save(S, TEXT("../AIHEROES_ZoneExpedition_HACK"), Message));
    if (!TestTrue(TEXT("First isolated slot A writes with readback"),
        FAIHeroesZoneExpeditionStore::Save(S, Prefix, Message))) { Clean(); return false; }
    auto Loaded = FAIHeroesZoneExpeditionStore::Load(Prefix);
    TestTrue(TEXT("First validated snapshot loads"), Loaded.bOk);
    TestEqual(TEXT("Initial sequence"), Loaded.Sequence, 1);
    TestEqual(TEXT("First copy A"), Loaded.Slot, A);
    TestEqual(TEXT("Stored generated revision"), Loaded.State.MapRevision, S.MapRevision);
    TestEqual(TEXT("Stored fog equals initial"), Loaded.State.DiscoveredMask, uint32(15));

    FString Error;
    if (!TestTrue(TEXT("New turn produces valid second snapshot"),
        FAIHeroesZoneExpeditionRules::EndPlayerDay(S, Error))) { Clean(); return false; }
    if (!TestTrue(TEXT("Second snapshot goes to alternate B"),
        FAIHeroesZoneExpeditionStore::Save(S, Prefix, Message))) { Clean(); return false; }
    Loaded = FAIHeroesZoneExpeditionStore::Load(Prefix);
    TestEqual(TEXT("Newest sequence selected"), Loaded.Sequence, 2);
    TestEqual(TEXT("Newest valid copy is B"), Loaded.Slot, B);
    TestEqual(TEXT("New day persisted"), Loaded.State.Day, 2);

    auto* Forgery = NewObject<UAIHeroesZoneExpeditionSave>(GetTransientPackage());
    Forgery->SaveSequence = 3;
    Forgery->State = S;
    Forgery->State.MapRevision ^= 1u;
    TestTrue(TEXT("Test-only forged B slot written"),
        UGameplayStatics::SaveGameToSlot(Forgery, B, 0));
    Loaded = FAIHeroesZoneExpeditionStore::Load(Prefix);
    TestTrue(TEXT("Backup A recovered from corrupt newer copy"), Loaded.bOk);
    TestTrue(TEXT("Explicit recovery flag"), Loaded.bRecovered);
    TestEqual(TEXT("Recovered last valid sequence"), Loaded.Sequence, 1);
    TestEqual(TEXT("Recovered original day"), Loaded.State.Day, 1);
    TestTrue(TEXT("A survives any B corruption"),
        UGameplayStatics::DoesSaveGameExist(A, 0));
    TestFalse(TEXT("Cannot load own v2 snapshot from legacy prefix"),
        FAIHeroesZoneExpeditionStore::Load(TEXT("AIHEROES_Campaign_Resilient_v1")).bOk);
    Clean();
    TestFalse(TEXT("GUID test slots cleaned up"),
        UGameplayStatics::DoesSaveGameExist(A, 0) ||
        UGameplayStatics::DoesSaveGameExist(B, 0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FZoneExpeditionSeedsTest,
    "AIHeroes.ZoneExpedition.SeedSweepKnownMapAndBounds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FZoneExpeditionSeedsTest::RunTest(const FString&)
{
    FString Error;
    int32 Valid = 0;
    for (uint32 Seed = 0; Seed < 128; ++Seed)
    {
        for (int32 Branches = 0; Branches <= 4; ++Branches)
        {
            FAIHeroesZoneExpeditionState State;
            FAIHeroesAdventureGraph View;
            if (FAIHeroesZoneExpeditionRules::Create(Seed, Branches, State, Error) &&
                FAIHeroesZoneExpeditionRules::Validate(State, Error) &&
                FAIHeroesZoneExpeditionRules::PlayerView(State, View, Error) &&
                View.Nodes.Num() == 4 && View.Links.Num() <= 8)
                ++Valid;
        }
    }
    TestEqual(TEXT("640 distinct map states support isolated readable frontier"),
        Valid, 640);
    return true;
}
#endif
