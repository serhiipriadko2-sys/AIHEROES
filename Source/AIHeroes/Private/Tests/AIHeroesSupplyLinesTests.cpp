#include "AIHeroesSupplyLines.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesGameMode.h"
#include "AIHeroesHUDLayout.h"
#include "AIHeroesTravelRules.h"
#include "AIHeroesWorldNode.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

namespace
{
FAIHeroesSupplyAccess ReadyForest()
{
    FAIHeroesSupplyAccess A;
    A.Site = EAIHeroesFieldSite::ForestCamp;
    A.bSourceDiscovered = true;
    A.bHeadquartersOwned = true;
    A.bSourceOwned = true;
    A.bOutpostOwned = true;
    A.bRoadOpen = true;
    A.bHeroAtHeadquarters = true;
    A.MovesLeft = 4;
    return A;
}

FAIHeroesFieldDwellings StockForest(int32 Count=5)
{
    auto S = FAIHeroesFieldDwellingRules::Initial();
    S.Forest.bClaimed = true;
    S.Forest.Available = Count;
    return S;
}

TArray<FAIHeroesArmyStack> StartingArmy()
{
    return {
        FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3),
        FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7)};
}

struct FSupplyWorld
{
    UWorld* World = nullptr;
    AAIHeroesGameMode* Game = nullptr;
    AAIHeroesWorldNode* Forest = nullptr;
    AAIHeroesWorldNode* Outpost = nullptr;

    FSupplyWorld()
    {
        const UWorld::InitializationValues Values =
            UWorld::InitializationValues()
                .AllowAudioPlayback(false).CreatePhysicsScene(false)
                .CreateNavigation(false).CreateAISystem(false)
                .ShouldSimulatePhysics(false);
        World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None,
            nullptr, true, ERHIFeatureLevel::Num, &Values);
        if (!World) return;
        if (GEngine)
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        Game = World->SpawnActor<AAIHeroesGameMode>();
        if (!Game) return;
        const TCHAR* Ids[] = {
            TEXT("HQ"), TEXT("FOREST"), TEXT("ORE"), TEXT("OUTPOST"), TEXT("RUINS")};
        const EAIHeroesNodeType Types[] = {
            EAIHeroesNodeType::Headquarters, EAIHeroesNodeType::ForestMine,
            EAIHeroesNodeType::OreMine, EAIHeroesNodeType::Outpost,
            EAIHeroesNodeType::Ruins};
        for (int32 I = 0; I < 5; ++I)
        {
            const FVector Position = I == 0
                ? Game->GetHeroStrategicLocation()
                : FVector(-400.f + I * 130.f, -300.f + I * 100.f, 110.f);
            AAIHeroesWorldNode* Node =
                World->SpawnActor<AAIHeroesWorldNode>(
                    Position, FRotator::ZeroRotator);
            if (!Node) return;
            Node->Configure(Types[I], Ids[I], {});
            Game->Nodes.Add(Node);
            if (I == 1) Forest = Node;
            if (I == 3) Outpost = Node;
        }
        Game->SelectedNode = Forest;
        Game->PlayerArmy = StartingArmy();
        Game->FieldDwellings = StockForest(10);
        Game->Gold = 240;
        Game->Wood = 35;
        Game->Ore = 30;
        Game->MovesLeft = 4;
    }

    ~FSupplyWorld()
    {
        if (World)
        {
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }
    }
    bool Valid() const
    {
        return World && Game && Forest && Outpost && Game->Nodes.Num() == 5;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHConvoyFogAndRaidTest,
    "AIHeroes.Supply.ScoutFirstAndOutpostBlockade",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHConvoyFogAndRaidTest::RunTest(const FString& Parameters)
{
    auto Access = ReadyForest();
    const auto Sites = StockForest(13);
    const auto Army = StartingArmy();
    const int32 Gold=240, Wood=35, Ore=30;

    Access.bSourceDiscovered = false;
    auto Quote = FAIHeroesSupplyLines::Quote(
        Access, Sites, Gold, Wood, Ore, Army);
    TestFalse(TEXT("Hidden source cannot be quoted"), Quote.bReady);
    TestTrue(TEXT("Fog message exposes no secret stock"),
        !Quote.Summary().Contains(TEXT("13")) &&
        Quote.Summary().Contains(TEXT("scout")));
    Access.bSourceDiscovered = true;
    Access.bSourceOwned = false;
    TestFalse(TEXT("Unsecured territory cannot dispatch"),
        FAIHeroesSupplyLines::Quote(Access, Sites, Gold, Wood, Ore, Army).bReady);
    Access.bSourceOwned = true;
    Access.bOutpostOwned = false;
    Quote = FAIHeroesSupplyLines::Quote(Access, Sites, Gold, Wood, Ore, Army);
    TestFalse(TEXT("Hostile OUTPOST prevents remote deliveries"), Quote.bReady);
    TestTrue(TEXT("Explicit blockaded reason"), Quote.Summary().Contains(TEXT("OUTPOST")));
    Access.bOutpostOwned = true;
    Access.bRoadOpen = false;
    TestFalse(TEXT("Decorative road does not override authoritative route"),
        FAIHeroesSupplyLines::Quote(Access, Sites, Gold, Wood, Ore, Army).bReady);
    Access.bRoadOpen = true;
    Access.bHeroAtHeadquarters = false;
    TestFalse(TEXT("Hero outside HQ cannot receive convoy"),
        FAIHeroesSupplyLines::Quote(Access, Sites, Gold, Wood, Ore, Army).bReady);
    Access.bHeroAtHeadquarters = true;
    Access.MovesLeft = 0;
    TestFalse(TEXT("No remaining movement prevents convoy"),
        FAIHeroesSupplyLines::Quote(Access, Sites, Gold, Wood, Ore, Army).bReady);
    Access.MovesLeft = 4;
    auto Unclaimed = Sites;
    Unclaimed.Forest.bClaimed = false;
    TestFalse(TEXT("Unclaimed field cannot remotely sell troops"),
        FAIHeroesSupplyLines::Quote(Access, Unclaimed, Gold, Wood, Ore, Army).bReady);
    TestTrue(TEXT("Known secured and claimed route is ready"),
        FAIHeroesSupplyLines::Quote(Access, Sites, Gold, Wood, Ore, Army).bReady);
    TestEqual(TEXT("Quote never changes original stock"), Sites.Forest.Available, 13);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHConvoyAtomicTest,
    "AIHeroes.Supply.PurchaseAtomicAndNoDuplicate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHConvoyAtomicTest::RunTest(const FString& Parameters)
{
    auto Access = ReadyForest();
    auto Sites = StockForest(5);
    int32 Moves=4, Gold=240, Wood=35, Ore=30;
    auto Army = StartingArmy();
    const auto Quote = FAIHeroesSupplyLines::Quote(
        Access, Sites, Gold, Wood, Ore, Army);
    TestTrue(TEXT("Quote is ready"), Quote.bReady);
    TestEqual(TEXT("One standard militia pack"), Quote.Units, 5);
    TestEqual(TEXT("Price includes 15-gold escort fee"), Quote.Gold, 65);
    TestEqual(TEXT("Canonical five-wood cost"), Quote.Wood, 5);
    TestEqual(TEXT("One movement point, no free teleport"), Quote.MovementCost, 1);
    TestEqual(TEXT("Quote is strictly read-only"), Sites.Forest.Available, 5);
    TestEqual(TEXT("Quote cannot consume movement"), Moves, 4);

    FString Error;
    TestTrue(TEXT("Explicit convoy delivery commits once"),
        FAIHeroesSupplyLines::Dispatch(Access, Sites, Moves,
            Gold, Wood, Ore, Army, Error));
    TestEqual(TEXT("Consistent stock commit"), Sites.Forest.Available, 0);
    TestEqual(TEXT("Full purchase plus escort deducted"), Gold, 175);
    TestEqual(TEXT("Forest resource cost paid"), Wood, 30);
    TestEqual(TEXT("Irrelevant ore not charged"), Ore, 30);
    TestEqual(TEXT("Shipment spends one strategic move"), Moves, 3);
    TestEqual(TEXT("Army gains five militia"), Army[0].Count(), 13);
    const auto After = Sites;
    TestFalse(TEXT("Replay cannot duplicate spent cargo"),
        FAIHeroesSupplyLines::Dispatch(Access, Sites, Moves,
            Gold, Wood, Ore, Army, Error));
    TestTrue(TEXT("Stock unchanged on replay"), Sites == After);
    TestEqual(TEXT("No double-gold charge"), Gold, 175);
    TestEqual(TEXT("No double-army grant"), Army[0].Count(), 13);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHConvoyOreTest,
    "AIHeroes.Supply.OreAerieDistinctAndDailyMoveLimit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHConvoyOreTest::RunTest(const FString& Parameters)
{
    auto Access = ReadyForest();
    Access.Site = EAIHeroesFieldSite::OreAerie;
    Access.MovesLeft = 1;
    auto Sites = StockForest(5);
    Sites.Ore.bClaimed = true;
    Sites.Ore.Available = 2;
    int32 Gold=240, Wood=35, Ore=30, Moves=1;
    auto Army = StartingArmy();
    auto Quote = FAIHeroesSupplyLines::Quote(
        Access, Sites, Gold, Wood, Ore, Army);
    TestTrue(TEXT("Ore Aerie supply available independently"), Quote.bReady);
    TestEqual(TEXT("Scout freight price is 70+15 gold"), Quote.Gold, 85);
    TestEqual(TEXT("Ore pack costs four ore"), Quote.Ore, 4);
    TestEqual(TEXT("No forest wood charge"), Quote.Wood, 0);
    FString Error;
    TestTrue(TEXT("Exactly two scouts delivered"),
        FAIHeroesSupplyLines::Dispatch(Access, Sites, Moves,
            Gold, Wood, Ore, Army, Error));
    TestEqual(TEXT("Scout stack increased"), Army[1].Count(), 5);
    TestEqual(TEXT("Ore reserve consumed"), Sites.Ore.Available, 0);
    TestEqual(TEXT("Forest reserve untouched"), Sites.Forest.Available, 5);
    TestEqual(TEXT("Ore spent once"), Ore, 26);
    TestEqual(TEXT("Remaining movement zero"), Moves, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHConvoyFailClosedTest,
    "AIHeroes.Supply.MalformedArmyAndUnaffordableFailClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHConvoyFailClosedTest::RunTest(const FString& Parameters)
{
    auto Access = ReadyForest();
    auto Sites = StockForest(5);
    auto Army = StartingArmy();
    FString Error;
    int32 Moves=4, Gold=64, Wood=35, Ore=30;
    TestFalse(TEXT("64 gold cannot pay escorted pack"),
        FAIHeroesSupplyLines::Dispatch(Access, Sites, Moves,
            Gold, Wood, Ore, Army, Error));
    TestEqual(TEXT("No partial gold charge"), Gold, 64);
    TestEqual(TEXT("No stock loss"), Sites.Forest.Available, 5);
    TestEqual(TEXT("No movement loss"), Moves, 4);
    TestEqual(TEXT("No free troops"), Army[0].Count(), 8);

    Gold=240; Wood=4;
    TestFalse(TEXT("Not enough wood prevents convoy"),
        FAIHeroesSupplyLines::Dispatch(Access, Sites, Moves,
            Gold, Wood, Ore, Army, Error));
    TestEqual(TEXT("Gold untouched on wood failure"), Gold, 240);
    Wood=35;
    Army[0].TotalHealth = 499950;
    TestFalse(TEXT("Stack near max HP prevents overflow"),
        FAIHeroesSupplyLines::Dispatch(Access, Sites, Moves,
            Gold, Wood, Ore, Army, Error));
    TestEqual(TEXT("No stock loss when army cannot fit"), Sites.Forest.Available, 5);
    TestEqual(TEXT("No gold on army failure"), Gold, 240);
    Army = StartingArmy();
    Access.MovesLeft=3;
    TestFalse(TEXT("Stale access cannot use an old budget"),
        FAIHeroesSupplyLines::Dispatch(Access, Sites, Moves,
            Gold, Wood, Ore, Army, Error));
    TestTrue(TEXT("Stale quote requires refresh"), Error.Contains(TEXT("stale")));
    TestEqual(TEXT("No movement on stale budget"), Moves, 4);
    Access.MovesLeft=4;
    Access.Site=static_cast<EAIHeroesFieldSite>(255);
    TestFalse(TEXT("Invalid site is not defaulted to forest"),
        FAIHeroesSupplyLines::Dispatch(Access, Sites, Moves,
            Gold, Wood, Ore, Army, Error));
    TestEqual(TEXT("Bad site never changes forest"), Sites.Forest.Available, 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHConvoyUIRegressionTest,
    "AIHeroes.Supply.MobileDesktopTouchHitboxNoOverlaps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHConvoyUIRegressionTest::RunTest(const FString& Parameters)
{
    for (const FVector2D Viewport : {
        FVector2D(1280.f, 720.f),
        FVector2D(960.f, 540.f),
        FVector2D(640.f, 720.f),
        FVector2D(480.f, 800.f),
        FVector2D(360.f, 640.f),
        FVector2D(320.f, 640.f)})
    {
        const auto Convoy = FAIHeroesHUDLayout::Rect(205, Viewport);
        const FVector2D Center(
            Convoy.X + Convoy.W / 2.f, Convoy.Y + Convoy.H / 2.f);
        TestTrue(TEXT("CONVOY touch target is fully visible"),
            Convoy.W >= 100.f && Convoy.H >= 28.f &&
            Convoy.X >= 8.f && Convoy.Y >= 0.f &&
            Convoy.X + Convoy.W <= Viewport.X &&
            Convoy.Y + Convoy.H <= Viewport.Y);
        TestEqual(TEXT("Touch and mouse share action205"),
            FAIHeroesHUDLayout::HitTest(Center, Viewport), 205);
        for (int32 Peer : {202, 203, 204})
            TestFalse(TEXT("Supply and old utility do not overlap"),
                FAIHeroesHUDLayout::Rect(Peer, Viewport).Contains(Center));
        TestEqual(TEXT("Strategic convoy hidden in live battle"),
            FAIHeroesHUDLayout::Rect(205, Viewport, true).W, 0.f);
        TestEqual(TEXT("Same location is inert in live battle"),
            FAIHeroesHUDLayout::HitTest(Center, Viewport, true), INDEX_NONE);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHConvoyGameIntegrationTest,
    "AIHeroes.Supply.GameModeQuoteRequoteAndCommitHumanOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHConvoyGameIntegrationTest::RunTest(const FString& Parameters)
{
    FSupplyWorld F;
    if (!TestTrue(TEXT("Disposable five-node world created"), F.Valid()))
        return false;
    auto& G = *F.Game;
    const int32 GoldStart = G.Gold, WoodStart = G.Wood;
    G.RequestSupplyConvoy();
    TestFalse(TEXT("Cannot dispatch with hostile FOREST"), G.IsSupplyConvoyPending());
    TestEqual(TEXT("Hostile source cannot charge"), G.Gold, GoldStart);
    F.Forest->SetOwned(true);
    G.RequestSupplyConvoy();
    TestFalse(TEXT("Hostile OUTPOST blocks convoy quote"), G.IsSupplyConvoyPending());
    TestTrue(TEXT("Explains danger instead of silently transferring"),
        G.LastEvent.Contains(TEXT("OUTPOST")));
    F.Outpost->SetOwned(true);
    G.RequestSupplyConvoy();
    TestTrue(TEXT("First player action only generates a quote"),
        G.IsSupplyConvoyPending());
    TestTrue(TEXT("Quote visible in event"),
        G.LastEvent.Contains(TEXT("CONVOY QUOTED")));
    TestEqual(TEXT("Quote does not spend gold"), G.Gold, GoldStart);
    TestEqual(TEXT("Quote does not decrement moves"), G.MovesLeft, 4);
    TestEqual(TEXT("Quote does not change army"), G.PlayerArmy[0].Count(), 8);

    // Simulate legitimate intervening resource state change. Even a previously
    // signed quote can only trigger a fresh proposal, never a stale purchase.
    ++G.Wood;
    G.RequestSupplyConvoy();
    TestTrue(TEXT("Intervening state requires a new quote"),
        G.LastEvent.Contains(TEXT("CONVOY QUOTED")));
    TestEqual(TEXT("Requote still spends zero gold"), G.Gold, GoldStart);
    TestEqual(TEXT("Requote still has no soldiers"), G.PlayerArmy[0].Count(), 8);
    G.RequestSupplyConvoy();
    TestEqual(TEXT("Third human action confirms delivery exactly once"),
        G.PlayerArmy[0].Count(), 13);
    TestEqual(TEXT("Escort and packet deducted"), G.Gold, GoldStart-65);
    TestEqual(TEXT("One wood-neutral state change then pack"),
        G.Wood, WoodStart+1-5);
    TestEqual(TEXT("One movement point consumed"), G.MovesLeft, 3);
    TestEqual(TEXT("Field cargo depleted by five"), G.FieldDwellings.Forest.Available, 5);
    TestFalse(TEXT("Quote cleared after successful purchase"), G.IsSupplyConvoyPending());
    TestTrue(TEXT("Receipt says one convoy delivered"),
        G.LastEvent.Contains(TEXT("CONVOY ARRIVED")));

    G.RequestSupplyConvoy(); // With 5 remaining, this should be a new quote.
    TestEqual(TEXT("No replayed convoy on first subsequent press"),
        G.PlayerArmy[0].Count(), 13);
    TestEqual(TEXT("No duplicate charge on first subsequent press"),
        G.Gold, GoldStart-65);
    return true;
}
#endif
