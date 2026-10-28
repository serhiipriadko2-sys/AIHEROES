#include "AIHeroesBaseRules.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesGameMode.h"
#include "AIHeroesCampaignSave.h"
#include "AIHeroesCampaignRules.h"
#include "AIHeroesSaveSystem.h"
#include "AIHeroesHUDLayout.h"
#include "AIHeroesWorldNode.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "UObject/UnrealType.h"

namespace
{
struct FBaseTestWorld
{
    UWorld* World = nullptr;
    AAIHeroesGameMode* Game = nullptr;
    AAIHeroesWorldNode* HQ = nullptr;
    AAIHeroesWorldNode* Forest = nullptr;
    FBaseTestWorld()
    {
        const UWorld::InitializationValues Values = UWorld::InitializationValues()
            .AllowAudioPlayback(false).CreatePhysicsScene(false)
            .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
        World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
            true, ERHIFeatureLevel::Num, &Values);
        if (!World) return;
        if (GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        Game = World->SpawnActor<AAIHeroesGameMode>();
        if (!Game) return;
        const TCHAR* Names[] = {TEXT("HQ"), TEXT("FOREST"), TEXT("ORE"), TEXT("OUTPOST"), TEXT("RUINS")};
        const EAIHeroesNodeType Types[] = {EAIHeroesNodeType::Headquarters, EAIHeroesNodeType::ForestMine,
            EAIHeroesNodeType::OreMine, EAIHeroesNodeType::Outpost, EAIHeroesNodeType::Ruins};
        for (int32 I = 0; I < 5; ++I)
        {
            const FVector At = I == 0 ? Game->GetHeroStrategicLocation() : FVector(-100.f + I * 150.f, -500.f, 110.f);
            AAIHeroesWorldNode* Node = World->SpawnActor<AAIHeroesWorldNode>(At, FRotator::ZeroRotator);
            if (!Node) return;
            Node->Configure(Types[I], Names[I], {});
            Game->Nodes.Add(Node);
            if (I == 0) HQ = Node;
            if (I == 1) Forest = Node;
        }
        Game->SelectedNode = HQ;
        Game->PlayerArmy = {FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3),
            FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7)};
    }
    ~FBaseTestWorld()
    {
        if (World)
        {
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }
    }
    bool Valid() const { return World && Game && HQ && Forest && Game->Nodes.Num() == 5; }
    UAIHeroesCampaignSave* Snapshot() const
    {
        UAIHeroesCampaignSave* S = NewObject<UAIHeroesCampaignSave>(GetTransientPackage());
        S->BaseTier = Game->BaseTier;
        S->Gold = Game->Gold; S->Wood = Game->Wood; S->Ore = Game->Ore;
        S->Day = Game->Day; S->HeroNodeId = TEXT("HQ");
        S->PlayerArmy = Game->PlayerArmy;
        for (const AAIHeroesWorldNode* Node : Game->Nodes)
        {
            FAIHeroesSavedNode N;
            N.NodeId = FName(*Node->GetDisplayName()); N.Type = Node->NodeType;
            N.bOwned = Node->bOwned; N.Defenders = Node->Defenders;
            S->NodeStates.Add(N);
        }
        return S;
    }
    void FinishBattle()
    {
        Game->FastFinishLiveBattle();
        for (int32 I = 0; I < 40 && Game->GetLiveBattlePhase() == EAIHeroesLiveBattlePhase::Running; ++I)
            Game->Tick(1.f / 60.f);
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesBasePricingTest, "AIHeroes.Base.PricingAndAtomicPurchase",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesBasePricingTest::RunTest(const FString& Parameters)
{
    int32 Tier = 0, Gold = 100, Wood = 10, Ore = 9;
    FString Error;
    TestFalse(TEXT("Missing ore refuses the complete transaction"),
        FAIHeroesBaseRules::TryUpgrade(Tier, Gold, Wood, Ore, true, Error));
    TestEqual(TEXT("Refused transaction retains gold"), Gold, 100);
    TestEqual(TEXT("Refused transaction retains wood"), Wood, 10);
    TestEqual(TEXT("Refused transaction retains tier"), Tier, 0);
    Ore = 10;
    TestFalse(TEXT("No HQ permission blocks purchase"),
        FAIHeroesBaseRules::TryUpgrade(Tier, Gold, Wood, Ore, false, Error));
    TestTrue(TEXT("Exact budget purchases one tier"),
        FAIHeroesBaseRules::TryUpgrade(Tier, Gold, Wood, Ore, true, Error));
    TestEqual(TEXT("Exact budget leaves zero gold; no immediate income"), Gold, 0);
    TestEqual(TEXT("First tier"), Tier, 1);
    TestEqual(TEXT("First-tier future income"), FAIHeroesBaseRules::DailyGold(Tier), 60);
    Gold = Wood = Ore = 1000;
    TestTrue(TEXT("Tier two available"), FAIHeroesBaseRules::TryUpgrade(Tier, Gold, Wood, Ore, true, Error));
    TestTrue(TEXT("Tier three available"), FAIHeroesBaseRules::TryUpgrade(Tier, Gold, Wood, Ore, true, Error));
    TestEqual(TEXT("All later prices charged"), Gold, 520);
    TestEqual(TEXT("Max daily income"), FAIHeroesBaseRules::DailyGold(Tier), 100);
    TestFalse(TEXT("No free fourth tier"), FAIHeroesBaseRules::TryUpgrade(Tier, Gold, Wood, Ore, true, Error));
    TestEqual(TEXT("Maxed state retains funds"), Gold, 520);
    for (int32 Invalid : {-1, 4, MAX_int32})
    {
        Tier = Invalid;
        TestFalse(TEXT("Unknown levels are not upgraded"), FAIHeroesBaseRules::TryUpgrade(Tier, Gold, Wood, Ore, true, Error));
        TestFalse(TEXT("Unknown levels have no upgrade quote"), FAIHeroesBaseRules::UpgradeCost(Tier).bAvailable);
        TestEqual(TEXT("Unknown levels have no invented income"), FAIHeroesBaseRules::DailyGold(Tier), 0);
    }
    Tier = 0; Gold = -1;
    TestFalse(TEXT("Negative balances rejected"), FAIHeroesBaseRules::TryUpgrade(Tier, Gold, Wood, Ore, true, Error));
    Gold = MAX_int32;
    TestFalse(TEXT("Over-limit balances rejected"), FAIHeroesBaseRules::TryUpgrade(Tier, Gold, Wood, Ore, true, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesBaseLocationTest, "AIHeroes.Base.LocationIncomeAndAdvice",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesBaseLocationTest::RunTest(const FString& Parameters)
{
    FBaseTestWorld F;
    if (!TestTrue(TEXT("Disposable campaign exists"), F.Valid())) return false;
    auto& G = *F.Game;
    G.AskAdvisor();
    TestTrue(TEXT("HQ advice exposes price at seed tier"), G.LastHint.Contains(TEXT("100G")));
    TestEqual(TEXT("Consultation never spends gold"), G.Gold, 240);
    G.UpgradeHeadquarters();
    TestEqual(TEXT("One purchase changes level"), G.BaseTier, 1);
    TestEqual(TEXT("One purchase charges exact gold"), G.Gold, 140);
    TestEqual(TEXT("Purchase does not consume moves"), G.MovesLeft, 4);
    TestEqual(TEXT("One visible addition"), F.HQ->GetVisibleHeadquartersParts(), 1);
    TestFalse(TEXT("Old dossier is stale after upgrade"), G.IsAdvisorDossierFresh());
    G.EndTurn();
    TestEqual(TEXT("Next day pays 60, not 40 or 100"), G.Gold, 200);
    F.Forest->SetOwned(true);
    G.Nodes[2]->SetOwned(true); G.Nodes[3]->SetOwned(true);
    const int32 BeforeWood = G.Wood, BeforeOre = G.Ore;
    G.EndTurn();
    TestEqual(TEXT("Owned outpost stacks with HQ income"), G.Gold, 290);
    TestEqual(TEXT("Forest still pays wood"), G.Wood, BeforeWood + 15);
    TestEqual(TEXT("Ore mine still pays ore"), G.Ore, BeforeOre + 12);
    G.InteractWithNode(F.Forest);
    G.SelectNode(F.HQ);
    G.UpgradeHeadquarters();
    TestEqual(TEXT("Selecting HQ remotely is not enough"), G.BaseTier, 1);
    TestEqual(TEXT("Remote refusal costs nothing"), G.Gold, 290);
    G.EndTurn(); G.InteractWithNode(F.HQ); G.UpgradeHeadquarters();
    TestEqual(TEXT("Return home unlocks second upgrade"), G.BaseTier, 2);
    TestEqual(TEXT("Visual follows gameplay level"), F.HQ->GetHeadquartersTier(), 2);
    TestEqual(TEXT("Economic track does not install protocols"), G.ProtocolTier, 0);
    TestEqual(TEXT("Economic track does not load model slots"), G.ModelSlotTier, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesBaseLocksTest, "AIHeroes.Base.ModalLocksAndBattleConflict",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesBaseLocksTest::RunTest(const FString& Parameters)
{
    FBaseTestWorld F;
    if (!TestTrue(TEXT("Disposable campaign exists"), F.Valid())) return false;
    auto& G = *F.Game;
    G.ToggleAdvisorDossier(); G.UpgradeHeadquarters();
    TestEqual(TEXT("Dossier blocks construction at authority layer"), G.BaseTier, 0);
    TestEqual(TEXT("Dossier block preserves resources"), G.Gold, 240);
    G.CloseAdvisorDossier();
    F.Forest->Defenders = {FAIHeroesArmyStack(TEXT("Wolf"), 4, 9, 3, 1, 8)};
    G.InteractWithNode(F.Forest);
    if (!TestTrue(TEXT("Live fight begins"), G.IsLiveBattleActive())) return false;
    G.UpgradeHeadquarters();
    TestEqual(TEXT("Live combat blocks construction even while HQ is selected"), G.BaseTier, 0);
    G.ToggleLiveBattlePause(); G.UpgradeHeadquarters();
    TestEqual(TEXT("Pause does not release strategic authority"), G.Gold, 240);
    G.BaseTier = 1; // Simulated unsupported concurrent mutation.
    F.FinishBattle();
    TestFalse(TEXT("Base-tier drift aborts battle commit"), G.IsLiveBattleActive());
    TestEqual(TEXT("Conflicted battle does not charge movement"), G.MovesLeft, 4);
    TestEqual(TEXT("Conflicted battle grants no reward"), G.DataFragments, 0);
    TestEqual(TEXT("Conflicted battle creates no journal"), G.BattleJournal.Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesBasePersistenceTest, "AIHeroes.Base.SaveValidationAndRecovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesBasePersistenceTest::RunTest(const FString& Parameters)
{
    FBaseTestWorld F;
    if (!TestTrue(TEXT("Disposable campaign exists"), F.Valid())) return false;
    UAIHeroesCampaignSave* S = F.Snapshot();
    FString Error;
    TestEqual(TEXT("Legacy/default level is zero"), S->BaseTier, 0);
    TestTrue(TEXT("Baseline schema-1 snapshot is valid"), FAIHeroesCampaignRules::Validate(*S, Error));
    S->BaseTier = -1;
    TestFalse(TEXT("Negative saved base tier rejected"), FAIHeroesCampaignRules::Validate(*S, Error));
    S->BaseTier = 4;
    TestFalse(TEXT("Unknown saved base tier rejected"), FAIHeroesCampaignRules::Validate(*S, Error));
    const FString Prefix = TEXT("AIHEROES_BaseTest_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
    S->BaseTier = 1;
    TestTrue(TEXT("First tier snapshot saves with read-back"), FAIHeroesSaveSystem::Save(*S, Prefix, FString(), Error));
    S->BaseTier = 3;
    TestTrue(TEXT("Higher tier rotates to second snapshot"), FAIHeroesSaveSystem::Save(*S, Prefix, FString(), Error));
    FAIHeroesLoadedCampaign Loaded = FAIHeroesSaveSystem::Load(Prefix, FString());
    if (TestNotNull(TEXT("Newest tier snapshot loads"), Loaded.Snapshot))
        TestEqual(TEXT("Tier retained through actual disk serialization"), Loaded.Snapshot->BaseTier, 3);
    // Only our GUID-named test file is corrupted; no human slot is touched.
    S->BaseTier = 9;
    TestTrue(TEXT("Write invalid test-only alternate"),
        UGameplayStatics::SaveGameToSlot(S, FAIHeroesSaveSystem::SlotB(Prefix), 0));
    Loaded = FAIHeroesSaveSystem::Load(Prefix, FString());
    if (TestNotNull(TEXT("Last valid snapshot recovered"), Loaded.Snapshot))
        TestEqual(TEXT("Recovery restores actual prior base level"), Loaded.Snapshot->BaseTier, 1);
    TestTrue(TEXT("Invalid level activates recovery indicator"), Loaded.bRecovered);
    for (const FString& Slot : {FAIHeroesSaveSystem::SlotA(Prefix), FAIHeroesSaveSystem::SlotB(Prefix)})
        TestTrue(TEXT("Only test-owned save is removed"), UGameplayStatics::DeleteGameInSlot(Slot, 0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesBaseLimitsTest, "AIHeroes.Base.DayAndResourceBounds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesBaseLimitsTest::RunTest(const FString& Parameters)
{
    FBaseTestWorld F;
    if (!TestTrue(TEXT("Disposable campaign exists"), F.Valid())) return false;
    auto& G = *F.Game;
    G.Gold = FAIHeroesBaseRules::MaxResource - 39; G.MovesLeft = 1;
    G.EndTurn();
    TestEqual(TEXT("Overflow refuses the whole day"), G.Day, 1);
    TestEqual(TEXT("Refused day does not reset movement"), G.MovesLeft, 1);
    G.Gold = 0; G.Day = FAIHeroesBaseRules::MaxDay;
    G.EndTurn();
    TestEqual(TEXT("Max day cannot advance"), G.Day, FAIHeroesBaseRules::MaxDay);
    TestEqual(TEXT("Max day cannot mint income repeatedly"), G.Gold, 0);
    G.Day = 1; G.BaseTier = 99; G.EndTurn();
    TestEqual(TEXT("Invalid tier fails closed"), G.Gold, 0);
    G.BaseTier = 3; G.EndTurn();
    TestEqual(TEXT("Valid recovered state can advance"), G.Day, 2);
    TestEqual(TEXT("Valid tier pays its stated income"), G.Gold, 100);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesBaseVisualTest, "AIHeroes.Base.VisualAndInputIsolation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesBaseVisualTest::RunTest(const FString& Parameters)
{
    FBaseTestWorld F;
    if (!TestTrue(TEXT("Disposable campaign exists"), F.Valid())) return false;
    for (int32 Tier : {0, 1, 2, 3, 0})
    {
        TestTrue(TEXT("Valid visual tier applied"), F.HQ->SetHeadquartersTier(Tier));
        TestEqual(TEXT("Exact number of visible upgrade parts"), F.HQ->GetVisibleHeadquartersParts(), Tier);
    }
    TestFalse(TEXT("Invalid visual tier rejected"), F.HQ->SetHeadquartersTier(4));
    TestFalse(TEXT("Mine cannot become a headquarters"), F.Forest->SetHeadquartersTier(1));
    for (int32 Width : {640, 960, 1280})
    {
        const FVector2D Viewport(Width, 720);
        const FAIHeroesHUDActionRect R = FAIHeroesHUDLayout::Rect(201, Viewport);
        const FVector2D Center(R.X + R.W / 2, R.Y + R.H / 2);
        TestTrue(TEXT("HQ control is visible"), R.W >= 40 && R.X >= 0 && R.X + R.W <= Width);
        TestEqual(TEXT("Mouse/touch resolve HQ action 201"), FAIHeroesHUDLayout::HitTest(Center, Viewport), 201);
        TestFalse(TEXT("HQ cannot swallow model button"), FAIHeroesHUDLayout::Rect(6, Viewport).Contains(Center));
        TestEqual(TEXT("Live battle hides construction"), FAIHeroesHUDLayout::Rect(201, Viewport, true).W, 0.f);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesBaseLegacyTest, "AIHeroes.Base.LegacyPropertyAbsent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesBaseLegacyTest::RunTest(const FString& Parameters)
{
    // Construct the previous tagged-property payload without touching global
    // reflection flags or reading/modifying a human campaign file.
    struct FLegacyArchive : FObjectAndNameAsStringProxyArchive
    {
        mutable bool bSkippedBase = false;
        explicit FLegacyArchive(FArchive& Inner)
            : FObjectAndNameAsStringProxyArchive(Inner, false) { ArNoDelta = true; }
        virtual bool ShouldSkipProperty(const FProperty* Property) const override
        {
            if (Property && Property->GetFName() == FName(TEXT("BaseTier")))
            {
                bSkippedBase = true;
                return true;
            }
            return FObjectAndNameAsStringProxyArchive::ShouldSkipProperty(Property);
        }
    };
    FBaseTestWorld F;
    if (!TestTrue(TEXT("Disposable campaign exists"), F.Valid())) return false;
    UAIHeroesCampaignSave* Before = F.Snapshot();
    Before->BaseTier = 3;
    Before->Day = 7;
    TArray<uint8> Bytes;
    FMemoryWriter Writer(Bytes, true);
    FLegacyArchive Saving(Writer);
    Before->Serialize(Saving);
    TestTrue(TEXT("The new property was actually omitted"), Saving.bSkippedBase);
    TestFalse(TEXT("Legacy payload writes without archive error"), Saving.IsError());
    UAIHeroesCampaignSave* After = NewObject<UAIHeroesCampaignSave>(GetTransientPackage());
    FMemoryReader Reader(Bytes, true);
    FObjectAndNameAsStringProxyArchive Loading(Reader, true);
    After->Serialize(Loading);
    TestFalse(TEXT("Legacy payload reads without archive error"), Loading.IsError());
    TestEqual(TEXT("Absent field uses base level zero, never invents upgrades"), After->BaseTier, 0);
    TestEqual(TEXT("Other old fields survive"), After->Day, 7);
    FString Error;
    TestTrue(TEXT("Restored legacy-shaped payload passes campaign validation"),
        FAIHeroesCampaignRules::Validate(*After, Error));
    return true;
}
#endif
