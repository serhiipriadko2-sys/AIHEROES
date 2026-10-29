#include "AIHeroesTravelRules.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesAdvisor.h"
#include "AIHeroesAIProposalGate.h"
#include "AIHeroesRecruitmentAdvisor.h"
#include "AIHeroesRoutePlanner.h"
#include "AIHeroesGameMode.h"
#include "AIHeroesCampaignRules.h"
#include "AIHeroesCampaignSave.h"
#include "AIHeroesSaveSystem.h"
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
#include <limits>

namespace
{
FAIHeroesAdvisorSnapshot RoadScenario()
{
    FAIHeroesAdvisorSnapshot S;
    S.TravelRuleVersion = 2; S.ProtocolTier = 1; S.MovesLeft = 4;
    S.Gold = 240; S.Wood = 35; S.Ore = 30;
    S.HeroPosition = FVector(-650.f, 0.f, 155.f);
    S.PlayerArmy = {FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3),
        FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7)};
    const TCHAR* Ids[] = {TEXT("HQ"), TEXT("FOREST"), TEXT("ORE"), TEXT("OUTPOST"), TEXT("RUINS")};
    const EAIHeroesNodeType Types[] = {EAIHeroesNodeType::Headquarters, EAIHeroesNodeType::ForestMine,
        EAIHeroesNodeType::OreMine, EAIHeroesNodeType::Outpost, EAIHeroesNodeType::Ruins};
    const FVector Positions[] = {S.HeroPosition, FVector(-100,-650,110), FVector(150,600,120),
        FVector(700,-300,140), FVector(750,650,165)};
    for (int32 I=0; I<5; ++I)
    {
        FAIHeroesAdvisorNode N;
        N.Id = Ids[I]; N.Type = Types[I]; N.Location = Positions[I]; N.bOwned = I == 0;
        S.Nodes.Add(N);
    }
    S.Nodes[1].Guards = {FAIHeroesArmyStack(TEXT("Wolf"), 4, 9, 3, 1, 8)};
    S.Nodes[2].Guards = {FAIHeroesArmyStack(TEXT("Bandit"), 6, 10, 4, 2, 4)};
    S.Nodes[3].Guards = {FAIHeroesArmyStack(TEXT("Raider"), 6, 11, 5, 2, 5),
        FAIHeroesArmyStack(TEXT("Raider"), 4, 11, 5, 2, 5)};
    S.Nodes[4].Guards = {FAIHeroesArmyStack(TEXT("Guardian"), 7, 24, 7, 4, 2)};
    return S;
}
UAIHeroesCampaignSave* RoadSave(int32 Version)
{
    const auto A = RoadScenario();
    auto* S = NewObject<UAIHeroesCampaignSave>(GetTransientPackage());
    S->TravelRuleVersion = Version; S->HeroNodeId = TEXT("HQ"); S->PlayerArmy = A.PlayerArmy;
    for (const auto& N : A.Nodes)
    {
        FAIHeroesSavedNode R; R.NodeId=N.Id; R.Type=N.Type; R.bOwned=N.bOwned; R.Defenders=N.Guards;
        S->NodeStates.Add(R);
    }
    return S;
}
struct FRoadWorld
{
    UWorld* World = nullptr;
    AAIHeroesGameMode* Game = nullptr;
    FRoadWorld()
    {
        const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false)
            .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
        World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
            ERHIFeatureLevel::Num, &Values);
        if (!World) return;
        if (GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        Game = World->SpawnActor<AAIHeroesGameMode>();
        if (!Game) return;
        const auto S=RoadScenario(); Game->TravelRuleVersion=2; Game->PlayerArmy=S.PlayerArmy;
        for (const auto& N : S.Nodes)
        {
            auto* A=World->SpawnActor<AAIHeroesWorldNode>(N.Location, FRotator::ZeroRotator);
            if (!A) return;
            A->Configure(N.Type, N.Id.ToString(), N.Guards); Game->Nodes.Add(A);
        }
    }
    ~FRoadWorld()
    {
        if(World) {World->DestroyWorld(false); if(GEngine) GEngine->DestroyWorldContext(World);}
    }
    bool Valid() const {return World && Game && Game->Nodes.Num()==5;}
    void Finish()
    {
        Game->FastFinishLiveBattle();
        for(int32 I=0; I<40 && Game->GetLiveBattlePhase()==EAIHeroesLiveBattlePhase::Running; ++I)
            Game->Tick(1.f/60.f);
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoadTopologyTest,"AIHeroes.TravelV2.TopologyGuardAndBudget",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRoadTopologyTest::RunTest(const FString& Parameters)
{
    auto S=RoadScenario();
    auto P=FAIHeroesTravelRules::Find(S,S.HeroPosition,TEXT("FOREST"),4);
    TestTrue(TEXT("Forest is a valid guarded destination"),P.IsReachableThisTurn() && P.bBattleAtDestination);
    TestEqual(TEXT("Authored road price"),P.TotalCost,2);
    P=FAIHeroesTravelRules::Find(S,S.HeroPosition,TEXT("OUTPOST"),4);
    TestTrue(TEXT("Cannot transit hostile forest or ore"),P.Status==EAIHeroesAdventurePathStatus::Unreachable);
    S.Nodes[1].Guards.Reset(); S.Nodes[1].bOwned=true;
    P=FAIHeroesTravelRules::Find(S,S.HeroPosition,TEXT("OUTPOST"),2);
    TestTrue(TEXT("Known path can exceed budget"),P.Status==EAIHeroesAdventurePathStatus::NeedsMoreMoves);
    TestEqual(TEXT("HQ-forest-outpost price"),P.TotalCost,3);
    TestEqual(TEXT("Exact missing budget"),P.MissingMoves,1);
    TestEqual(TEXT("Full route includes transit site"),P.Nodes.Num(),3);
    if(P.Nodes.Num()==3) TestEqual(TEXT("Transit is forest"),P.Nodes[1],FName(TEXT("FOREST")));
    S.TravelRuleVersion=1;
    TestTrue(TEXT("Old campaign can still travel directly"),
        FAIHeroesTravelRules::Find(S,S.HeroPosition,TEXT("RUINS"),4).IsReachableThisTurn());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoadOverDayWaypointTest,
    "AIHeroes.TravelV2.OverDayPathHasReachableOwnedStop",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRoadOverDayWaypointTest::RunTest(const FString&)
{
    auto S=RoadScenario();
    // From owned ORE to guarded OUTPOST: the neutral RUINS are hostile, so
    // ROAD V2 must use ORE > HQ > FOREST > OUTPOST, costing 2+2+1=5.
    S.Nodes[1].Guards.Reset();S.Nodes[1].bOwned=true;
    S.Nodes[2].Guards.Reset();S.Nodes[2].bOwned=true;
    const FVector From=S.Nodes[2].Location;
    const auto Full=FAIHeroesTravelRules::Find(
        S,From,FName(TEXT("OUTPOST")),4);
    if(!TestEqual(TEXT("Over-day path requires a staged movement"),
        Full.Status,EAIHeroesAdventurePathStatus::NeedsMoreMoves))
        return false;
    TestEqual(TEXT("Route exceeds every full four-move day"),
        Full.TotalCost,5);
    TestEqual(TEXT("One missing move is not solvable by waiting"),
        Full.MissingMoves,1);
    TestEqual(TEXT("Full canonical route has four sites"),
        Full.Nodes.Num(),4);
    if(Full.Nodes.Num()==4)
    {
        TestEqual(TEXT("Owned headquarters is an intermediate node"),
            Full.Nodes[1],FName(TEXT("HQ")));
        TestEqual(TEXT("Owned forest is a subsequent stage"),
            Full.Nodes[2],FName(TEXT("FOREST")));
    }
    const auto First=FAIHeroesTravelRules::Find(
        S,From,FName(TEXT("HQ")),4);
    TestTrue(TEXT("Player can choose headquarters as a real legal stop"),
        First.IsReachableThisTurn());
    TestEqual(TEXT("First safe leg costs two"),
        First.TotalCost,2);
    TestTrue(TEXT("Player can choose forest as a real legal stop"),
        FAIHeroesTravelRules::Find(
            S,From,FName(TEXT("FOREST")),4).IsReachableThisTurn());
    TestEqual(TEXT("Read-only queries never spend strategic moves"),
        S.MovesLeft,4);
    TestFalse(TEXT("Guarded objective remains under enemy control"),
        S.Nodes[3].bOwned);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoadAdviceTest,"AIHeroes.TravelV2.AdvisorRecruitGateParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRoadAdviceTest::RunTest(const FString& Parameters)
{
    auto S=RoadScenario(); S.SelectedNodeId=TEXT("OUTPOST");
    TestTrue(TEXT("Strategy admits blocked road"),FAIHeroesAdvisor::Advise(S).Contains(TEXT("ROAD BLOCKED")));
    const auto Recr=FAIHeroesRecruitmentAdvisor::Evaluate(S);
    TestFalse(TEXT("Recruiter never treats blocked destination as reachable"),Recr.bTargetReachable);
    TestEqual(TEXT("No false purchase recommendation"),Recr.RecommendedIndex,INDEX_NONE);
    for(const auto& O:Recr.Options) TestFalse(TEXT("No forecast through guard"),O.bForecastAvailable);
    FAIHeroesProposalSnapshot Q; Q.Strategic=S; Q.Day=1; Q.StateRevision=1;
    auto Order=FAIHeroesAIProposalGate::SuggestSelected(Q);
    TestFalse(TEXT("Gate uses same connectivity"),FAIHeroesAIProposalGate::Validate(Q,Order).bAccepted);
    S.Nodes[1].bOwned=true; S.Nodes[1].Guards.Reset(); S.MovesLeft=3;
    TestTrue(TEXT("Advisor exposes exact price"),FAIHeroesAdvisor::Advise(S).Contains(TEXT("3 moves")));
    TestTrue(TEXT("Recruiter recognizes newly open route"),FAIHeroesRecruitmentAdvisor::Evaluate(S).bTargetReachable);
    Q.Strategic=S; ++Q.StateRevision; Order=FAIHeroesAIProposalGate::SuggestSelected(Q);
    const auto Verdict=FAIHeroesAIProposalGate::Validate(Q,Order);
    TestTrue(TEXT("Legal proposal still requires human confirmation"),Verdict.bAccepted && Verdict.bPlayerConfirmationRequired);
    Q.Strategic.MovesLeft=2;
    TestFalse(TEXT("Insufficient budget is not accepted"),FAIHeroesAIProposalGate::Validate(Q,Order).bAccepted);
    TestEqual(TEXT("Queries do not purchase"),S.Gold,240);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoadPlannerTest,"AIHeroes.TravelV2.TwoStopHypotheticalCapture",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRoadPlannerTest::RunTest(const FString& Parameters)
{
    auto S=RoadScenario(); S.ProtocolTier=3; S.Gold=0; S.Wood=0; S.Ore=500;
    S.PlayerArmy={FAIHeroesArmyStack(TEXT("Militia"),30,12,4,2,3),FAIHeroesArmyStack(TEXT("Scout"),20,8,6,1,7)};
    const auto Plans=FAIHeroesRoutePlanner::Rank(S,3);
    bool Found=false;
    for(const auto& P:Plans)
    {
        if(P.Stops.Num()==2 && P.Stops[0]==TEXT("FOREST") && P.Stops[1]==TEXT("OUTPOST"))
        { Found=true; TestEqual(TEXT("Two-step cost uses road after simulated capture"),P.TotalMoves,3); }
        TestTrue(TEXT("Every plan stays in day budget"),P.TotalMoves<=4);
    }
    TestTrue(TEXT("Forest unlocks outpost inside forecast branch"),Found);
    TestFalse(TEXT("Hypothetical capture never mutates snapshot ownership"),S.Nodes[1].bOwned);
    TestEqual(TEXT("Hypothetical capture never deletes real guards"),S.Nodes[1].Guards.Num(),1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoadExecutionTest,"AIHeroes.TravelV2.GameModeCostAndAtomicBattle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRoadExecutionTest::RunTest(const FString& Parameters)
{
    FRoadWorld F; if(!TestTrue(TEXT("Test world exists"),F.Valid())) return false;
    auto& G=*F.Game; auto* Outpost=G.Nodes[3].Get();
    G.SelectNode(Outpost); G.ConfirmSelectedNode();
    TestFalse(TEXT("Blocked road never begins battle"),G.IsLiveBattleActive());
    TestEqual(TEXT("Blocked road costs nothing"),G.MovesLeft,4);
    TestEqual(TEXT("Blocked cost is not disguised as zero"),G.GetSelectedMoveCost(),INDEX_NONE);
    G.Nodes[1]->Defenders.Reset(); G.Nodes[1]->SetOwned(true);
    G.MovesLeft=2; G.ConfirmSelectedNode();
    TestFalse(TEXT("Insufficient movement never starts battle"),G.IsLiveBattleActive());
    TestEqual(TEXT("Failed preflight preserves moves"),G.MovesLeft,2);
    G.PlayerArmy={FAIHeroesArmyStack(TEXT("Militia"),30,12,4,2,3),FAIHeroesArmyStack(TEXT("Scout"),20,8,6,1,7)};
    G.MovesLeft=4;
    const int32 Price=G.GetSelectedMoveCost(); TestEqual(TEXT("Visible price is three"),Price,3);
    G.ConfirmSelectedNode();
    if(!TestTrue(TEXT("Reachable battle starts"),G.IsLiveBattleActive())) return false;
    TestEqual(TEXT("Start does not charge early"),G.MovesLeft,4);
    F.Finish();
    TestTrue(TEXT("Result commits"),G.GetLiveBattlePhase()==EAIHeroesLiveBattlePhase::Result);
    TestEqual(TEXT("Charged price equals displayed price"),G.MovesLeft,4-Price);
    TestEqual(TEXT("One result record"),G.BattleJournal.Num(),1);
    G.FastFinishLiveBattle(); G.Tick(1.f); G.ReturnFromLiveBattle();
    TestEqual(TEXT("Cannot double charge"),G.MovesLeft,4-Price);
    TestEqual(TEXT("Owned current site costs zero"),G.GetSelectedMoveCost(),0);
    auto* Foreign=F.World->SpawnActor<AAIHeroesWorldNode>();
    if(!TestNotNull(TEXT("Foreign actor exists"),Foreign)) return false;
    Foreign->Configure(EAIHeroesNodeType::Ruins,TEXT("RUINS"),{});
    G.InteractWithNode(Foreign);
    TestFalse(TEXT("Matching display name cannot authorize a foreign actor"),Foreign->bOwned);
    TestEqual(TEXT("Foreign actor cannot spend moves"),G.MovesLeft,1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoadDriftTest,"AIHeroes.TravelV2.RuleDriftAbortsCommit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRoadDriftTest::RunTest(const FString& Parameters)
{
    FRoadWorld F; if(!TestTrue(TEXT("Test world exists"),F.Valid())) return false;
    auto& G=*F.Game; G.SelectNode(G.Nodes[1]); G.AskAdvisor();
    TestTrue(TEXT("Report initially fresh"),G.IsAdvisorDossierFresh());
    G.ConfirmSelectedNode();
    if(!TestTrue(TEXT("Battle begins"),G.IsLiveBattleActive())) return false;
    G.TravelRuleVersion=1; F.Finish();
    TestFalse(TEXT("Changed travel rules abort pending result"),G.IsLiveBattleActive());
    TestEqual(TEXT("Conflict does not spend moves"),G.MovesLeft,4);
    TestEqual(TEXT("Conflict gives no reward"),G.DataFragments,0);
    TestEqual(TEXT("Conflict has no battle journal"),G.BattleJournal.Num(),0);
    TestFalse(TEXT("Report recognizes changed rule version"),G.IsAdvisorDossierFresh());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoadInvalidTest,"AIHeroes.TravelV2.InvalidMapFailsClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRoadInvalidTest::RunTest(const FString& Parameters)
{
    for(int32 Case=0;Case<6;++Case)
    {
        auto S=RoadScenario();
        if(Case==0) S.Nodes[2].Id=TEXT("FOREST");
        if(Case==1) S.Nodes.Pop();
        if(Case==2) S.Nodes[2].Location=S.Nodes[0].Location;
        if(Case==3) S.Nodes[1].Location.X=std::numeric_limits<double>::infinity();
        if(Case==4) S.TravelRuleVersion=999;
        if(Case==5) S.Nodes[0].bOwned=false;
        const auto P=FAIHeroesTravelRules::Find(S,S.HeroPosition,TEXT("FOREST"),4);
        TestTrue(TEXT("Malformed map is invalid, not zero-cost ready"),P.Status==EAIHeroesAdventurePathStatus::InvalidInput);
        TestTrue(TEXT("Invalid input gives no executable route"),P.Nodes.IsEmpty());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoadSaveTest,"AIHeroes.TravelV2.SaveVersionAndRecovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRoadSaveTest::RunTest(const FString& Parameters)
{
    auto* S=RoadSave(1); FString Error;
    TestTrue(TEXT("Legacy travel is valid"),FAIHeroesCampaignRules::Validate(*S,Error));
    const FString Prefix=TEXT("AIHEROES_TravelTest_")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
    TestTrue(TEXT("Write legacy to owned temporary A"),FAIHeroesSaveSystem::Save(*S,Prefix,FString(),Error));
    S->TravelRuleVersion=2;
    TestTrue(TEXT("Write road campaign with read-back"),FAIHeroesSaveSystem::Save(*S,Prefix,FString(),Error));
    auto Loaded=FAIHeroesSaveSystem::Load(Prefix,FString());
    if(TestNotNull(TEXT("New road campaign loads"),Loaded.Snapshot))
        TestEqual(TEXT("Road version persists"),Loaded.Snapshot->TravelRuleVersion,2);
    S->TravelRuleVersion=99;
    TestFalse(TEXT("Unknown future rule version rejected"),FAIHeroesCampaignRules::Validate(*S,Error));
    TestTrue(TEXT("Corrupt only our own temporary B"),UGameplayStatics::SaveGameToSlot(S,FAIHeroesSaveSystem::SlotB(Prefix),0));
    Loaded=FAIHeroesSaveSystem::Load(Prefix,FString());
    if(TestNotNull(TEXT("Prior valid campaign recovered"),Loaded.Snapshot))
        TestEqual(TEXT("Recovery restores exact old rules"),Loaded.Snapshot->TravelRuleVersion,1);
    TestTrue(TEXT("Recovery reported"),Loaded.bRecovered);
    for(const auto& Slot:{FAIHeroesSaveSystem::SlotA(Prefix),FAIHeroesSaveSystem::SlotB(Prefix)})
        TestTrue(TEXT("Remove only GUID-owned test saves"),UGameplayStatics::DeleteGameInSlot(Slot,0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoadLegacyTest,"AIHeroes.TravelV2.AbsentPropertyKeepsLegacyRules",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRoadLegacyTest::RunTest(const FString& Parameters)
{
    struct FOldArchive : FObjectAndNameAsStringProxyArchive
    {
        mutable bool bSkipped=false;
        explicit FOldArchive(FArchive& A):FObjectAndNameAsStringProxyArchive(A,false){ArNoDelta=true;}
        virtual bool ShouldSkipProperty(const FProperty* P) const override
        {
            if(P && P->GetFName()==TEXT("TravelRuleVersion")){bSkipped=true;return true;}
            return FObjectAndNameAsStringProxyArchive::ShouldSkipProperty(P);
        }
    };
    auto* S=RoadSave(2); TArray<uint8> Bytes; FMemoryWriter Writer(Bytes,true); FOldArchive Saving(Writer);
    S->Serialize(Saving); TestTrue(TEXT("Property really absent"),Saving.bSkipped);
    auto* After=NewObject<UAIHeroesCampaignSave>(GetTransientPackage());
    FMemoryReader Reader(Bytes,true); FObjectAndNameAsStringProxyArchive Loading(Reader,true);
    After->Serialize(Loading);
    TestFalse(TEXT("Old-shaped payload reads without error"),Loading.IsError());
    TestEqual(TEXT("Old payload retains distance V1"),After->TravelRuleVersion,1);
    FString Error; TestTrue(TEXT("Recovered legacy campaign valid"),FAIHeroesCampaignRules::Validate(*After,Error));
    return true;
}
#endif
