#include "AIHeroesAdvisorDossier.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesGameMode.h"
#include "AIHeroesRecruitmentAdvisor.h"
#include "AIHeroesWorldNode.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

namespace
{
struct FDossierTestWorld
{
    UWorld* World = nullptr;
    AAIHeroesGameMode* Game = nullptr;
    AAIHeroesWorldNode* Node = nullptr;

    FDossierTestWorld()
    {
        const UWorld::InitializationValues Values = UWorld::InitializationValues()
            .AllowAudioPlayback(false).CreatePhysicsScene(false)
            .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
        World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
            true, ERHIFeatureLevel::Num, &Values);
        if (!World) return;
        if (GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        Game = World->SpawnActor<AAIHeroesGameMode>();
        Node = World->SpawnActor<AAIHeroesWorldNode>(
            FVector(-100.f, -650.f, 110.f), FRotator::ZeroRotator);
        if (!Game || !Node) return;
        Node->Configure(EAIHeroesNodeType::ForestMine, TEXT("FOREST"),
            {FAIHeroesArmyStack(TEXT("Wolf"), 4, 9, 3, 1, 8)});
        Game->Nodes.Add(Node);
        Game->SelectedNode = Node;
        Game->PlayerArmy = {
            FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3),
            FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7)
        };
    }

    ~FDossierTestWorld()
    {
        if (World)
        {
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }
    }

    bool IsValid() const { return World && Game && Node; }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesDossierRussianCopyTest,
    "AIHeroes.Dossier.RussianCopyAndUnicode",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesDossierRussianCopyTest::RunTest(const FString& Parameters)
{
    FAIHeroesAdvisorDossier D;
    D.Title = TEXT("Unknown source title");
    TestEqual(TEXT("Unknown report title is not fabricated"),
        FAIHeroesAdvisorDossierRules::DisplayTitle(D), D.Title);

    D.bObjectives = true;
    TestEqual(TEXT("Objectives use a Russian display title"),
        FAIHeroesAdvisorDossierRules::DisplayTitle(D),
        FString(TEXT("ЦЕЛИ КАМПАНИИ")));
    D.bObjectives = false;
    D.bRecruitment = true;
    TestEqual(TEXT("Recruitment uses a Russian display title"),
        FAIHeroesAdvisorDossierRules::DisplayTitle(D),
        FString(TEXT("СОВЕТ ПО АРМИИ")));
    D.bRecruitment = false;
    D.bProtocolLab = true;
    TestEqual(TEXT("Protocol analysis is not a purchase button"),
        FAIHeroesAdvisorDossierRules::DisplayTitle(D),
        FString(TEXT("СРАВНЕНИЕ ТАКТИК")));

    TestFalse(TEXT("Stale data never has the same status as fresh data"),
        FAIHeroesAdvisorDossierRules::FreshnessLabel(true) ==
        FAIHeroesAdvisorDossierRules::FreshnessLabel(false));
    TestTrue(TEXT("Stale data explicitly asks for refresh"),
        FAIHeroesAdvisorDossierRules::FreshnessLabel(false).Contains(TEXT("ОБНОВИТЕ")));
    for (int32 Action = FAIHeroesAdvisorDossierRules::PreviousAction;
        Action <= FAIHeroesAdvisorDossierRules::CloseAction; ++Action)
        TestFalse(TEXT("All four modal controls have Russian captions"),
            FAIHeroesAdvisorDossierRules::ActionLabel(Action).IsEmpty());
    TestEqual(TEXT("Unknown action never receives a misleading caption"),
        FAIHeroesAdvisorDossierRules::ActionLabel(999), FString());

    const FString Russian = TEXT("Сверхдлинноеназваниеперепроверкисохранений");
    const auto Wrapped = FAIHeroesAdvisorDossierRules::Wrap({Russian}, 16);
    TestTrue(TEXT("Cyrillic long token spans more than one line"), Wrapped.Num() > 1);
    TestEqual(TEXT("Cyrillic report content survives wrapping"),
        FString::Join(Wrapped, TEXT("")), Russian);
    for (const FString& Line : Wrapped)
        TestTrue(TEXT("Cyrillic report line respects column bound"), Line.Len() <= 16);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesDossierWrapTest,
    "AIHeroes.Dossier.WrapAndExplicitLimits",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesDossierWrapTest::RunTest(const FString& Parameters)
{
    const FString Token = FString::ChrN(83, TEXT('X'));
    const TArray<FString> Lines = FAIHeroesAdvisorDossierRules::Wrap({Token}, 16);
    TestEqual(TEXT("Long tokens wrap instead of being cut off"), FString::Join(Lines, TEXT("")), Token);
    for (const FString& Line : Lines)
        TestTrue(TEXT("Every line fits the requested character width"), Line.Len() <= 16);

    const auto Normal = FAIHeroesAdvisorDossierRules::Wrap(
        {TEXT("One two three"), TEXT("Four five six")}, 16);
    TestTrue(TEXT("Paragraph boundary is retained"), Normal.Contains(FString()));
    TestFalse(TEXT("Ordinary reports have no false limit warning"), Normal.Contains(TEXT("[REPORT LIMIT]")));
    const auto Oversized = FAIHeroesAdvisorDossierRules::Wrap(
        {FString::ChrN(FAIHeroesAdvisorDossierRules::MaxCharacters + 1, TEXT('Y'))}, 100);
    TestTrue(TEXT("Oversized input is explicitly marked, never silently cut"),
        Oversized.Contains(TEXT("[REPORT LIMIT]")));
    TArray<FString> TooManyParagraphs;
    TooManyParagraphs.Init(TEXT("paragraph"), 65);
    TestTrue(TEXT("Paragraph limit is explicitly marked"),
        FAIHeroesAdvisorDossierRules::Wrap(TooManyParagraphs, 40).Contains(TEXT("[REPORT LIMIT]")));
    TestEqual(TEXT("Empty report still has a safe page count"),
        FAIHeroesAdvisorDossierRules::PageCount(0, 0), 1);
    TestEqual(TEXT("Final partial page stays reachable"),
        FAIHeroesAdvisorDossierRules::PageCount(21, 10), 3);
    TestEqual(TEXT("Page clamps to existing final page"),
        FAIHeroesAdvisorDossierRules::ClampPage(20, 3), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesDossierLayoutTest,
    "AIHeroes.Dossier.LayoutAndModalHitTest",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesDossierLayoutTest::RunTest(const FString& Parameters)
{
    for (const FVector2D Viewport : {FVector2D(640.f, 480.f), FVector2D(1280.f, 720.f)})
    {
        const auto Layout = FAIHeroesAdvisorDossierRules::Layout(Viewport);
        TestTrue(TEXT("Dossier panel fits the viewport"),
            Layout.Panel.X >= 0.f && Layout.Panel.Y >= 0.f &&
            Layout.Panel.X + Layout.Panel.W <= Viewport.X &&
            Layout.Panel.Y + Layout.Panel.H <= Viewport.Y);
        TestTrue(TEXT("At least one report row is visible"), Layout.RowsPerPage >= 1);
        for (int32 Action = FAIHeroesAdvisorDossierRules::PreviousAction;
            Action <= FAIHeroesAdvisorDossierRules::CloseAction; ++Action)
        {
            const auto Rect = FAIHeroesAdvisorDossierRules::Button(Action, Viewport);
            TestEqual(TEXT("Rendered button geometry and hit testing agree"),
                FAIHeroesAdvisorDossierRules::HitTest(
                    FVector2D(Rect.X + Rect.W / 2.f, Rect.Y + Rect.H / 2.f), Viewport), Action);
        }
        TestEqual(TEXT("Backdrop clicks are consumed instead of reaching the map"),
            FAIHeroesAdvisorDossierRules::HitTest(FVector2D(1.f, 1.f), Viewport),
            FAIHeroesAdvisorDossierRules::ConsumeAction);
    }
    TestEqual(TEXT("Invalid viewport produces no panel"),
        FAIHeroesAdvisorDossierRules::Layout(FVector2D::ZeroVector).Panel.W, 0.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesDossierReadOnlyTest,
    "AIHeroes.Dossier.QueryFreshnessAndModalLocks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesDossierReadOnlyTest::RunTest(const FString& Parameters)
{
    FDossierTestWorld F;
    if (!TestTrue(TEXT("Disposable dossier world exists"), F.IsValid())) return false;
    AAIHeroesGameMode& G = *F.Game;
    const int32 Gold = G.Gold, Wood = G.Wood, Ore = G.Ore, Day = G.Day, Moves = G.MovesLeft;
    const int32 FriendlyHP = G.PlayerArmy[0].TotalHealth;
    const int32 GuardHP = F.Node->Defenders[0].TotalHealth;

    G.AskAdvisor();
    TestTrue(TEXT("Strategy query keeps the full advice paragraph"),
        G.GetAdvisorDossier().Paragraphs.Contains(G.LastHint));
    TestFalse(TEXT("Read-only query does not open the modal unexpectedly"), G.IsAdvisorDossierVisible());
    TestTrue(TEXT("New strategy report is fresh"), G.IsAdvisorDossierFresh());
    G.AskRecruitmentAdvisor();
    TestTrue(TEXT("Recruitment report records its category"), G.GetAdvisorDossier().bRecruitment);
    TestTrue(TEXT("Recruitment summary is preserved without HUD truncation"),
        G.GetAdvisorDossier().Paragraphs.Contains(G.LastHint));
    TestTrue(TEXT("Full price catalogue is accessible before protocol unlock"),
        FString::Join(G.GetAdvisorDossier().Paragraphs, TEXT(" ")).Contains(TEXT("5 Militia: 50G+5W")));

    G.ToggleAdvisorDossier();
    TestTrue(TEXT("Player explicitly opens the dossier"), G.IsAdvisorDossierVisible());
    G.ChangeAdvisorDossierPage(1, 3);
    TestEqual(TEXT("Page navigation changes only transient UI state"), G.GetAdvisorDossierPage(), 1);
    G.RecruitMilitia(); G.RecruitScouts(); G.UpgradeProtocol(); G.UpgradeModelSlot();
    G.EndTurn(); G.CycleSaveSlot(); G.InteractWithNode(F.Node); G.PlanRoute();
    TestEqual(TEXT("Modal commands do not spend gold"), G.Gold, Gold);
    TestEqual(TEXT("Modal commands do not spend wood"), G.Wood, Wood);
    TestEqual(TEXT("Modal commands do not spend ore"), G.Ore, Ore);
    TestEqual(TEXT("Modal commands do not advance day"), G.Day, Day);
    TestEqual(TEXT("Modal commands do not spend movement"), G.MovesLeft, Moves);
    TestEqual(TEXT("Modal commands do not unlock protocols"), G.ProtocolTier, 0);
    TestEqual(TEXT("Modal commands do not change armies"), G.PlayerArmy[0].TotalHealth, FriendlyHP);
    TestEqual(TEXT("Modal commands do not change guards"), F.Node->Defenders[0].TotalHealth, GuardHP);
    TestFalse(TEXT("Modal commands cannot launch a battle"), G.IsLiveBattleActive());
    TestFalse(TEXT("Modal commands cannot capture a node"), F.Node->bOwned);

    G.Gold += 1; // Simulate an external state change rather than a supported modal command.
    TestFalse(TEXT("Budget changes make the stored evidence stale"), G.IsAdvisorDossierFresh());
    G.RefreshAdvisorDossier();
    TestTrue(TEXT("Read-only refresh returns to the open report"), G.IsAdvisorDossierVisible());
    TestTrue(TEXT("Refresh captures the current evidence"), G.IsAdvisorDossierFresh());
    TestTrue(TEXT("Refresh preserves the recruitment category"), G.GetAdvisorDossier().bRecruitment);
    TestEqual(TEXT("Refresh issues no purchase"), G.Gold, Gold + 1);
    TestEqual(TEXT("Refresh resets pagination for the new snapshot"), G.GetAdvisorDossierPage(), 0);
    G.CloseAdvisorDossier();
    TestFalse(TEXT("Close releases only the UI modal"), G.IsAdvisorDossierVisible());
    TestEqual(TEXT("Close does not issue an order"), G.PlayerArmy[0].TotalHealth, FriendlyHP);
    return true;
}
#endif
