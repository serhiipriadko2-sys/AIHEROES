#include "AIHeroesGameMode.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesBattleRecord.h"
#include "AIHeroesLiveBattleStage.h"
#include "AIHeroesWorldNode.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Misc/AutomationTest.h"

namespace
{
struct FLiveBattleWorld
{
    UWorld* World = nullptr;
    AAIHeroesGameMode* Game = nullptr;
    AAIHeroesWorldNode* Node = nullptr;

    FLiveBattleWorld(bool bWeakPlayer = false)
    {
        const UWorld::InitializationValues Values = UWorld::InitializationValues()
            .AllowAudioPlayback(false).CreatePhysicsScene(false)
            .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
        World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
            true, ERHIFeatureLevel::Num, &Values);
        if (!World) return;
        if (GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        Game = World->SpawnActor<AAIHeroesGameMode>();
        Node = World->SpawnActor<AAIHeroesWorldNode>(FVector(-100.f, -650.f, 110.f), FRotator::ZeroRotator);
        if (!Game || !Node) return;
        Node->Configure(EAIHeroesNodeType::ForestMine, TEXT("FOREST"),
            {FAIHeroesArmyStack(TEXT("Wolf"), bWeakPlayer ? 40 : 4, 9, 3, 1, 8)});
        Game->PlayerArmy = {
            FAIHeroesArmyStack(TEXT("Militia"), bWeakPlayer ? 1 : 8, 12, 4, 2, 3),
            FAIHeroesArmyStack(TEXT("Scout"), bWeakPlayer ? 0 : 3, 8, 6, 1, 7)};
        Game->Nodes.Add(Node);
        Game->SelectedNode = Node;
    }
    ~FLiveBattleWorld()
    {
        if (World)
        {
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }
    }
    bool IsValid() const { return World && Game && Node; }
    void Finish()
    {
        Game->FastFinishLiveBattle();
        for (int32 Frame = 0; Frame < 40 &&
            Game->GetLiveBattlePhase() == EAIHeroesLiveBattlePhase::Running; ++Frame)
        {
            Game->Tick(1.f / 60.f);
        }
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesLiveAtomicCommitTest,
    "AIHeroes.LiveBattle.AtomicCommitAndLocks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesLiveAtomicCommitTest::RunTest(const FString& Parameters)
{
    FLiveBattleWorld F;
    if (!TestTrue(TEXT("Disposable game world exists"), F.IsValid())) return false;
    AAIHeroesGameMode& G = *F.Game;
    const int32 Gold = G.Gold, Wood = G.Wood, Ore = G.Ore, Moves = G.MovesLeft;
    const int32 HP = G.PlayerArmy[0].TotalHealth;
    G.InteractWithNode(F.Node);
    if (!TestTrue(TEXT("Confirmed attack starts live session"), G.IsLiveBattleActive())) return false;
    TestEqual(TEXT("Begin has no future actions"), G.GetLiveBattleActionCount(), 0);
    TestEqual(TEXT("Begin does not charge movement"), G.MovesLeft, Moves);
    TestFalse(TEXT("Begin does not capture"), F.Node->bOwned);
    G.ToggleLiveBattlePause();
    G.Tick(30.f);
    TestEqual(TEXT("Paused battle calculates no action"), G.GetLiveBattleActionCount(), 0);
    G.RecruitMilitia(); G.RecruitScouts(); G.UpgradeProtocol(); G.UpgradeModelSlot();
    G.EndTurn(); G.CycleSaveSlot(); G.SaveCampaign(); G.LoadCampaign();
    G.InteractWithNode(F.Node); G.SelectNode(nullptr); G.PlanRoute(); G.PlayLastBattleIn3D();
    TestEqual(TEXT("Method locks preserve gold"), G.Gold, Gold);
    TestEqual(TEXT("Method locks preserve wood"), G.Wood, Wood);
    TestEqual(TEXT("Method locks preserve ore"), G.Ore, Ore);
    TestEqual(TEXT("Method locks preserve day"), G.Day, 1);
    TestEqual(TEXT("Method locks preserve protocol"), G.ProtocolTier, 0);
    TestEqual(TEXT("Method locks preserve army"), G.PlayerArmy[0].TotalHealth, HP);
    TestEqual(TEXT("Method locks preserve slot"), G.ActiveSaveSlot, 1);
    F.Finish();
    TestTrue(TEXT("Fast finish reaches result, not a second simulation"),
        G.GetLiveBattlePhase() == EAIHeroesLiveBattlePhase::Result);
    TestTrue(TEXT("Result captures node"), F.Node->bOwned);
    TestEqual(TEXT("New record uses V2"), G.LastBattleRecord.RuleVersion, 2);
    TestEqual(TEXT("One journal entry"), G.BattleJournal.Num(), 1);
    const int32 FinalMoves = G.MovesLeft, FinalData = G.DataFragments;
    G.FastFinishLiveBattle(); G.Tick(5.f); G.ConfirmSelectedNode();
    TestEqual(TEXT("Repeated finish cannot charge twice"), G.MovesLeft, FinalMoves);
    TestEqual(TEXT("Repeated finish cannot reward twice"), G.DataFragments, FinalData);
    TestEqual(TEXT("Repeated finish cannot duplicate journal"), G.BattleJournal.Num(), 1);
    G.ReturnFromLiveBattle(); G.ReturnFromLiveBattle();
    TestFalse(TEXT("Return releases modal state"), G.IsLiveBattleActive());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesLiveTamperTest,
    "AIHeroes.LiveBattle.PrecommitConflict",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesLiveTamperTest::RunTest(const FString& Parameters)
{
    FLiveBattleWorld F;
    if (!TestTrue(TEXT("Disposable world exists"), F.IsValid())) return false;
    const int32 Moves = F.Game->MovesLeft;
    const int32 HP = F.Game->PlayerArmy[0].TotalHealth;
    F.Game->InteractWithNode(F.Node);
    F.Game->Gold += 1; // Simulate an external writer, not a supported input command.
    F.Finish();
    TestFalse(TEXT("Conflict aborts the live transaction"), F.Game->IsLiveBattleActive());
    TestEqual(TEXT("Aborted battle never charges movement"), F.Game->MovesLeft, Moves);
    TestEqual(TEXT("Aborted battle never applies casualties"), F.Game->PlayerArmy[0].TotalHealth, HP);
    TestEqual(TEXT("Aborted battle never appends journal"), F.Game->BattleJournal.Num(), 0);
    TestFalse(TEXT("Aborted battle never captures node"), F.Node->bOwned);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesLiveDefeatTest,
    "AIHeroes.LiveBattle.DefeatAndRepeat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesLiveDefeatTest::RunTest(const FString& Parameters)
{
    FLiveBattleWorld F(true);
    if (!TestTrue(TEXT("Disposable world exists"), F.IsValid())) return false;
    const FVector Position = F.Game->GetHeroStrategicLocation();
    F.Game->InteractWithNode(F.Node); F.Finish();
    TestTrue(TEXT("Defeat still produces a committed result"),
        F.Game->GetLiveBattlePhase() == EAIHeroesLiveBattlePhase::Result);
    TestTrue(TEXT("Weak army loses"), F.Game->LastBattleRecord.bDefeat);
    TestFalse(TEXT("Defeat does not capture"), F.Node->bOwned);
    TestTrue(TEXT("Defeat leaves hero outside"), F.Game->GetHeroStrategicLocation().Equals(Position));
    TestEqual(TEXT("Defeat grants no data"), F.Game->DataFragments, 0);
    F.Game->ReturnFromLiveBattle();
    F.Game->InteractWithNode(F.Node);
    TestFalse(TEXT("Destroyed army cannot launch another fight"), F.Game->IsLiveBattleActive());
    F.Game->RecruitMilitia(); F.Game->RecruitScouts(); F.Game->EndTurn();
    F.Game->InteractWithNode(F.Node); F.Finish();
    TestTrue(TEXT("Another session is independent and completes"),
        F.Game->GetLiveBattlePhase() == EAIHeroesLiveBattlePhase::Result);
    TestEqual(TEXT("Each session writes one journal item"), F.Game->BattleJournal.Num(), 2);
    F.Game->ReturnFromLiveBattle();
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesLivePacingTest,
    "AIHeroes.LiveBattle.FrameRateSpeedAndFastParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesLivePacingTest::RunTest(const FString& Parameters)
{
    FAIHeroesBattleRecord Reference;
    for (int32 Variant = 0; Variant < 4; ++Variant)
    {
        FLiveBattleWorld F;
        if (!TestTrue(TEXT("Disposable world exists"), F.IsValid())) return false;
        F.Game->ProtocolTier = 3;
        F.Game->InteractWithNode(F.Node);
        if (Variant == 0) F.Finish();
        else
        {
            for (int32 Speed = 1; Speed < Variant; ++Speed) F.Game->CycleLiveBattleSpeed();
            const float Dt = Variant == 1 ? 1.f / 20.f : 1.f / 120.f;
            for (int32 Frame = 0; Frame < 15000 &&
                F.Game->GetLiveBattlePhase() == EAIHeroesLiveBattlePhase::Running; ++Frame)
            {
                if (Frame == 7)
                {
                    F.Game->ToggleLiveBattlePause();
                    const int32 Actions = F.Game->GetLiveBattleActionCount();
                    F.Game->Tick(100.f);
                    for (TActorIterator<AAIHeroesLiveBattleStage> It(F.World); It; ++It) It->Tick(100.f);
                    TestEqual(TEXT("Pause does not advance engine"), F.Game->GetLiveBattleActionCount(), Actions);
                    F.Game->ToggleLiveBattlePause();
                }
                F.Game->Tick(Dt);
                for (TActorIterator<AAIHeroesLiveBattleStage> It(F.World); It; ++It) It->Tick(Dt);
            }
        }
        TestTrue(TEXT("Pacing variant reaches result"),
            F.Game->GetLiveBattlePhase() == EAIHeroesLiveBattlePhase::Result);
        if (Variant == 0) Reference = F.Game->LastBattleRecord;
        else TestTrue(TEXT("Frame rate, speed, pause and fast completion preserve exact evidence"),
            FAIHeroesBattleRecordRules::Equivalent(Reference, F.Game->LastBattleRecord));
        F.Game->ReturnFromLiveBattle();
        int32 RemainingStages = 0;
        for (TActorIterator<AAIHeroesLiveBattleStage> It(F.World); It; ++It) ++RemainingStages;
        TestEqual(TEXT("Return leaves no active stage"), RemainingStages, 0);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesLiveLostWorldTest,
    "AIHeroes.LiveBattle.MissingStageAndNode",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesLiveLostWorldTest::RunTest(const FString& Parameters)
{
    {
        FLiveBattleWorld F;
        if (!TestTrue(TEXT("Disposable world exists"), F.IsValid())) return false;
        F.Game->InteractWithNode(F.Node);
        for (TActorIterator<AAIHeroesLiveBattleStage> It(F.World); It; ++It) It->Destroy();
        F.Finish();
        TestTrue(TEXT("Visual loss completes through the same logical session"),
            F.Game->GetLiveBattlePhase() == EAIHeroesLiveBattlePhase::Result);
        TestEqual(TEXT("Visual loss still commits exactly once"), F.Game->BattleJournal.Num(), 1);
        F.Game->ReturnFromLiveBattle();
    }
    {
        FLiveBattleWorld F;
        if (!TestTrue(TEXT("Disposable world exists"), F.IsValid())) return false;
        const int32 Moves = F.Game->MovesLeft;
        F.Game->InteractWithNode(F.Node);
        F.Node->Destroy();
        F.Finish();
        TestFalse(TEXT("Missing node aborts transaction"), F.Game->IsLiveBattleActive());
        TestEqual(TEXT("Missing node does not spend movement"), F.Game->MovesLeft, Moves);
        TestEqual(TEXT("Missing node does not append evidence"), F.Game->BattleJournal.Num(), 0);
    }
    return true;
}
#endif
