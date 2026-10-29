#include "AIHeroesRecruitmentStock.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesAdvisor.h"
#include "AIHeroesBasePanel.h"
#include "AIHeroesCampaignRules.h"
#include "AIHeroesCampaignSave.h"
#include "AIHeroesGameMode.h"
#include "AIHeroesRecruitmentAdvisor.h"
#include "AIHeroesSaveSystem.h"
#include "AIHeroesWorldNode.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"

namespace
{
UAIHeroesCampaignSave* NewStockSnapshot()
{
    UAIHeroesCampaignSave* S =
        NewObject<UAIHeroesCampaignSave>(GetTransientPackage());
    S->HeroNodeId = FName(TEXT("HQ"));
    S->SelectedNodeId = FName(TEXT("HQ"));
    S->PlayerArmy = {
        FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3),
        FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7)
    };
    const EAIHeroesNodeType Types[] = {
        EAIHeroesNodeType::Headquarters, EAIHeroesNodeType::ForestMine,
        EAIHeroesNodeType::OreMine, EAIHeroesNodeType::Outpost,
        EAIHeroesNodeType::Ruins
    };
    const TCHAR* Names[] = {
        TEXT("HQ"), TEXT("FOREST"), TEXT("ORE"), TEXT("OUTPOST"), TEXT("RUINS")
    };
    for (int32 I = 0; I < 5; ++I)
    {
        FAIHeroesSavedNode N;
        N.Type = Types[I];
        N.NodeId = FName(Names[I]);
        N.bOwned = I == 0;
        S->NodeStates.Add(N);
    }
    return S;
}

struct FRecruitmentStockWorld
{
    UWorld* World = nullptr;
    AAIHeroesGameMode* Game = nullptr;

    FRecruitmentStockWorld()
    {
        const UWorld::InitializationValues Values =
            UWorld::InitializationValues()
            .AllowAudioPlayback(false).CreatePhysicsScene(false)
            .CreateNavigation(false).CreateAISystem(false)
            .ShouldSimulatePhysics(false);
        World = UWorld::CreateWorld(
            EWorldType::Game, false, NAME_None, nullptr,
            true, ERHIFeatureLevel::Num, &Values);
        if (!World) return;
        if (GEngine)
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        Game = World->SpawnActor<AAIHeroesGameMode>();
        if (!Game) return;
        const EAIHeroesNodeType Types[] = {
            EAIHeroesNodeType::Headquarters, EAIHeroesNodeType::ForestMine,
            EAIHeroesNodeType::OreMine, EAIHeroesNodeType::Outpost,
            EAIHeroesNodeType::Ruins
        };
        const TCHAR* Names[] = {
            TEXT("HQ"), TEXT("FOREST"), TEXT("ORE"), TEXT("OUTPOST"), TEXT("RUINS")
        };
        for (int32 I = 0; I < 5; ++I)
        {
            const FVector Location = I == 0
                ? Game->GetHeroStrategicLocation()
                : FVector(150.f * I, -500.f, 110.f);
            AAIHeroesWorldNode* Node =
                World->SpawnActor<AAIHeroesWorldNode>(
                    Location, FRotator::ZeroRotator);
            if (!Node) return;
            Node->Configure(Types[I], Names[I], {});
            Game->Nodes.Add(Node);
        }
        Game->SelectedNode = Game->Nodes[0];
        Game->PlayerArmy = {
            FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3),
            FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7)
        };
        Game->Gold = 1000;
        Game->Wood = 100;
        Game->Ore = 100;
    }

    ~FRecruitmentStockWorld()
    {
        if (World)
        {
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }
    }

    bool Valid() const { return World && Game && Game->Nodes.Num() == 5; }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesReserveWeeklyTest,
    "AIHeroes.Recruitment.StockWeeklyCalendar",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesReserveWeeklyTest::RunTest(const FString& Parameters)
{
    const FAIHeroesRecruitmentStock Initial =
        FAIHeroesRecruitmentStockRules::Initial();
    TestEqual(TEXT("Starter Militia inventory"), Initial.Militia, 10);
    TestEqual(TEXT("Starter Scout inventory"), Initial.Scouts, 4);
    FAIHeroesRecruitmentStock After;
    FString Error;
    for (int32 Day : {1, 2, 3, 4, 5, 6, 8, 9, 13, 15})
    {
        if (!TestTrue(TEXT("Regular day is valid"),
            FAIHeroesRecruitmentStockRules::PlanNextDay(
                Day, 0, false, Initial, After, Error))) return false;
        TestTrue(TEXT("No production on regular days"), After == Initial);
    }
    if (!TestTrue(TEXT("Day 7 to 8 is a new weekly production boundary"),
        FAIHeroesRecruitmentStockRules::PlanNextDay(
            7, 0, false, Initial, After, Error))) return false;
    TestEqual(TEXT("Week two: +5 Militia"), After.Militia, 15);
    TestEqual(TEXT("Week two: +2 Scouts"), After.Scouts, 6);
    const FAIHeroesRecruitmentStock WeekTwo = After;
    if (!TestTrue(TEXT("Day 14 to 15 accrues another week"),
        FAIHeroesRecruitmentStockRules::PlanNextDay(
            14, 0, false, WeekTwo, After, Error))) return false;
    TestEqual(TEXT("Week three: accumulated Militia"), After.Militia, 20);
    TestEqual(TEXT("Week three: accumulated Scouts"), After.Scouts, 8);
    TestTrue(TEXT("Source snapshot is unchanged"), Initial.Militia == 10);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesReserveOwnershipTest,
    "AIHeroes.Recruitment.StockOwnershipUpgradeAndCaps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesReserveOwnershipTest::RunTest(const FString& Parameters)
{
    const FAIHeroesRecruitmentStock Start{30, 40};
    FAIHeroesRecruitmentStock Unowned, Owned;
    FString Error;
    TestTrue(TEXT("No outpost: HQ growth calculated"),
        FAIHeroesRecruitmentStockRules::PlanNextDay(
            7, 3, false, Start, Unowned, Error));
    TestTrue(TEXT("Owned outpost: independent Scout production bonus"),
        FAIHeroesRecruitmentStockRules::PlanNextDay(
            7, 3, true, Start, Owned, Error));
    TestEqual(TEXT("Tier 3 HQ gives eleven Militia/week"), Unowned.Militia, 41);
    TestEqual(TEXT("Tier 3 HQ gives three Scouts/week"), Unowned.Scouts, 43);
    TestEqual(TEXT("Outpost gives exactly two extra Scouts/week"), Owned.Scouts, 45);
    TestEqual(TEXT("Outpost does not produce additional Militia"),
        Owned.Militia, Unowned.Militia);

    const FAIHeroesRecruitmentStock NearCap{249, 119};
    TestTrue(TEXT("Growth succeeds at capacity limit"),
        FAIHeroesRecruitmentStockRules::PlanNextDay(
            14, 3, true, NearCap, Owned, Error));
    TestEqual(TEXT("Militia bounded to capacity"), Owned.Militia,
        FAIHeroesRecruitmentStockRules::MaxMilitia);
    TestEqual(TEXT("Scouts bounded to capacity"), Owned.Scouts,
        FAIHeroesRecruitmentStockRules::MaxScouts);
    TestFalse(TEXT("Out-of-range reserve denied"),
        FAIHeroesRecruitmentStockRules::PlanNextDay(
            7, 0, false, FAIHeroesRecruitmentStock{251, 4}, Owned, Error));
    TestFalse(TEXT("Failed plan cannot leak partial production"),
        FAIHeroesRecruitmentStockRules::IsValid(Owned) && Owned.Militia > 0);
    TestFalse(TEXT("Invalid HQ tier denied"),
        FAIHeroesRecruitmentStockRules::PlanNextDay(
            7, 4, false, Start, Owned, Error));
    TestFalse(TEXT("Campaign day limit denied"),
        FAIHeroesRecruitmentStockRules::PlanNextDay(
            FAIHeroesRecruitmentStockRules::MaxDay, 0,
            false, Start, Owned, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesReservePurchaseTest,
    "AIHeroes.Recruitment.StockNoFreeRecruitment",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesReservePurchaseTest::RunTest(const FString& Parameters)
{
    FAIHeroesRecruitmentStock Stock =
        FAIHeroesRecruitmentStockRules::Initial();
    FString Error;
    for (int32 I = 0; I < 2; ++I)
    {
        TestTrue(TEXT("Each 5-unit Militia package consumes five stock"),
            FAIHeroesRecruitmentStockRules::Consume(
                Stock, EAIHeroesRecruitChoice::Militia, 5, Error));
    }
    TestEqual(TEXT("Militia reserves exhausted"), Stock.Militia, 0);
    const FAIHeroesRecruitmentStock Before = Stock;
    TestFalse(TEXT("Third same-week package is blocked"),
        FAIHeroesRecruitmentStockRules::Consume(
            Stock, EAIHeroesRecruitChoice::Militia, 5, Error));
    TestTrue(TEXT("No partial stock deduction"), Stock == Before);
    TestTrue(TEXT("Out-of-stock error is actionable"),
        Error.Contains(TEXT("day 8")));
    TestTrue(TEXT("Scout reserve is separate"),
        FAIHeroesRecruitmentStockRules::Consume(
            Stock, EAIHeroesRecruitChoice::Scouts, 2, Error));
    TestEqual(TEXT("Scout remaining stock"), Stock.Scouts, 2);
    TestFalse(TEXT("Unknown product refused"),
        FAIHeroesRecruitmentStockRules::Consume(
            Stock, EAIHeroesRecruitChoice::KeepArmy, 1, Error));
    TestFalse(TEXT("Negative quantity refused"),
        FAIHeroesRecruitmentStockRules::Consume(
            Stock, EAIHeroesRecruitChoice::Scouts, -1, Error));
    TestEqual(TEXT("Refusal never changes scout stock"), Stock.Scouts, 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesReserveAdvisorTest,
    "AIHeroes.Recruitment.StockQuotesAndAdvisorNoPhantomOffers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesReserveAdvisorTest::RunTest(const FString& Parameters)
{
    FAIHeroesBasePanelState State;
    State.Gold = 1000;
    State.Wood = 100;
    State.Ore = 100;
    State.Data = 100;
    State.MilitiaReserve = 0;
    State.ScoutReserve = 0;
    auto Offers = FAIHeroesBasePanelRules::Offers(State);
    TestFalse(TEXT("Out-of-stock Militia quote cannot be confirmed"),
        Offers[1].CanBuy());
    TestFalse(TEXT("Out-of-stock Scouts quote cannot be confirmed"),
        Offers[2].CanBuy());
    TestTrue(TEXT("Quote describes missing recruits"),
        Offers[1].BlockReason.Contains(TEXT("No recruits")));
    State.MilitiaReserve = 10;
    State.ScoutReserve = 4;
    Offers = FAIHeroesBasePanelRules::Offers(State);
    TestTrue(TEXT("Existing correct packages remain available"),
        Offers[1].CanBuy() && Offers[2].CanBuy());

    FAIHeroesAdvisorSnapshot Input;
    Input.ProtocolTier = 0;
    Input.MovesLeft = 4;
    Input.Gold = 1000;
    Input.Wood = 100;
    Input.Ore = 100;
    Input.MilitiaReserve = 0;
    Input.ScoutReserve = 0;
    Input.PlayerArmy = {
        FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3)
    };
    auto Advice = FAIHeroesRecruitmentAdvisor::Evaluate(Input);
    TestTrue(TEXT("No cash-only hallucination of affordable recruits"),
        Advice.bValidInput);
    TestEqual(TEXT("Advisor keeps the no-purchase option"), Advice.Options.Num(), 3);
    if (Advice.Options.Num() != 3) return false;
    TestFalse(TEXT("Advisor rejects phantom Militia"), Advice.Options[1].bAffordable);
    TestFalse(TEXT("Advisor rejects phantom Scouts"), Advice.Options[2].bAffordable);
    TestTrue(TEXT("Advice displays zero stock explicitly"),
        Advice.Detail.Contains(TEXT("Stock: 0 M/0 S")));
    Input.MilitiaReserve = 10;
    Input.ScoutReserve = 4;
    Advice = FAIHeroesRecruitmentAdvisor::Evaluate(Input);
    TestTrue(TEXT("Available stock restores the valid offer"),
        Advice.Options[1].bAffordable && Advice.Options[2].bAffordable);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesReserveSaveTest,
    "AIHeroes.Recruitment.StockSaveMigrationAndRotation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesReserveSaveTest::RunTest(const FString& Parameters)
{
    UAIHeroesCampaignSave* S = NewStockSnapshot();
    FString Error;
    if (!TestNotNull(TEXT("Test-owned save snapshot allocated"), S)) return false;
    TestFalse(TEXT("Legacy v1 has no reserve marker"),
        S->bRecruitmentStockInitialized);
    TestTrue(TEXT("Legacy-shaped snapshot accepts missing stock fields"),
        FAIHeroesCampaignRules::Validate(*S, Error));
    S->bRecruitmentStockInitialized = true;
    S->MilitiaReserve = 9;
    S->ScoutReserve = 1;
    TestTrue(TEXT("New optional v1 fields valid without schema bump"),
        FAIHeroesCampaignRules::Validate(*S, Error));
    S->MilitiaReserve = -1;
    TestFalse(TEXT("Negative stock cannot enter saved game"),
        FAIHeroesCampaignRules::Validate(*S, Error));
    S->MilitiaReserve = 251;
    TestFalse(TEXT("Invented enormous stock cannot enter save"),
        FAIHeroesCampaignRules::Validate(*S, Error));
    S->MilitiaReserve = 9;
    S->bRecruitmentStockInitialized = false;
    TestFalse(TEXT("False marker cannot smuggle stock into legacy save"),
        FAIHeroesCampaignRules::Validate(*S, Error));

    S->bRecruitmentStockInitialized = true;
    const FString Prefix = TEXT("AIHEROES_StockQA_") +
        FGuid::NewGuid().ToString(EGuidFormats::Digits);
    if (!TestTrue(TEXT("GUID-isolated first save has verified write/read-back"),
        FAIHeroesSaveSystem::Save(*S, Prefix, FString(), Error))) return false;
    S->MilitiaReserve = 4;
    S->ScoutReserve = 0;
    if (!TestTrue(TEXT("Second save rotates without overwriting first"),
        FAIHeroesSaveSystem::Save(*S, Prefix, FString(), Error))) return false;
    const FAIHeroesLoadedCampaign Recent =
        FAIHeroesSaveSystem::Load(Prefix, FString());
    if (TestNotNull(TEXT("Newest save read-back succeeds"), Recent.Snapshot))
    {
        TestEqual(TEXT("Stock survives disk serialization"), Recent.Snapshot->MilitiaReserve, 4);
        TestEqual(TEXT("Empty Scout stock survives disk serialization"), Recent.Snapshot->ScoutReserve, 0);
        TestTrue(TEXT("Serialized reserve is distinguishable from old v1"),
            Recent.Snapshot->bRecruitmentStockInitialized);
    }
    for (const FString& Slot : {
        FAIHeroesSaveSystem::SlotA(Prefix),
        FAIHeroesSaveSystem::SlotB(Prefix)})
    {
        TestTrue(TEXT("Only own GUID test slots cleaned"),
            UGameplayStatics::DeleteGameInSlot(Slot, 0));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesReserveGameModeTest,
    "AIHeroes.Recruitment.StockGameModeWeeklyEndToEnd",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesReserveGameModeTest::RunTest(const FString& Parameters)
{
    FRecruitmentStockWorld F;
    if (!TestTrue(TEXT("Disposable map and game authority built"), F.Valid())) return false;
    auto& Game = *F.Game;
    TestEqual(TEXT("Initial Militia reserve"), Game.GetAvailableMilitia(), 10);
    TestEqual(TEXT("Initial Scouts reserve"), Game.GetAvailableScouts(), 4);

    const int32 GoldBefore = Game.Gold;
    Game.RecruitMilitia();
    Game.RecruitMilitia();
    TestEqual(TEXT("Two player orders use ten Militia"), Game.GetAvailableMilitia(), 0);
    TestEqual(TEXT("Each successful action charges normal price"),
        Game.Gold, GoldBefore - 100);
    TestEqual(TEXT("Player army grows by ten"), Game.PlayerArmy[0].Count(), 18);
    Game.RecruitMilitia();
    TestEqual(TEXT("Third order cannot invent Militia"), Game.PlayerArmy[0].Count(), 18);
    TestEqual(TEXT("Refused order cannot charge gold"), Game.Gold, GoldBefore - 100);
    TestTrue(TEXT("Refused order explains why"), Game.LastEvent.Contains(TEXT("only 0 units")));

    Game.EndTurn();
    TestEqual(TEXT("No production on ordinary day 1 to 2"), Game.GetAvailableMilitia(), 0);
    TestEqual(TEXT("Day one advances exactly once"), Game.Day, 2);

    // Directly fast-forward only this disposable game fixture's calendar.
    Game.Day = 7;
    Game.EndTurn();
    TestEqual(TEXT("Day eight adds Militia stock"), Game.GetAvailableMilitia(), 5);
    TestEqual(TEXT("Day eight adds Scouts stock"), Game.GetAvailableScouts(), 6);
    TestEqual(TEXT("Day eight is one coherent transaction"), Game.Day, 8);

    // The outpost is a distinct capturable bonus, not an ownership change
    // to global neutral-guard rules or an automatic player purchase.
    Game.Nodes[3]->Defenders.Empty();
    Game.Nodes[3]->SetOwned(true);
    Game.Day = 14;
    const int32 BeforeScouts = Game.GetAvailableScouts();
    Game.EndTurn();
    TestEqual(TEXT("Owned outpost boosts next-week scout stock by two"),
        Game.GetAvailableScouts(), BeforeScouts + 4);
    TestEqual(TEXT("World advances to day fifteen"), Game.Day, 15);
    return true;
}
#endif
