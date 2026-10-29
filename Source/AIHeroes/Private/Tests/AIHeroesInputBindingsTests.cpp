#include "AIHeroesPlayerController.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesGameMode.h"
#include "AIHeroesRecruitmentAdvisor.h"
#include "AIHeroesWorldNode.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "InputCoreTypes.h"
#include "Misc/AutomationTest.h"

namespace
{
// A real authoritative GameMode lets production controller delegates resolve
// UGameplayStatics::GetGameMode without fake controller overrides or user saves.
struct FFieldInputWorld
{
    UWorld* World = nullptr;
    AAIHeroesGameMode* Game = nullptr;
    AAIHeroesPlayerController* Controller = nullptr;
    AAIHeroesWorldNode* Forest = nullptr;
    AAIHeroesWorldNode* HQ = nullptr;
    AAIHeroesWorldNode* Hostile = nullptr;

    FFieldInputWorld()
    {
        const UWorld::InitializationValues Values = UWorld::InitializationValues()
            .AllowAudioPlayback(false).CreatePhysicsScene(false)
            .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
        World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
            true, ERHIFeatureLevel::Num, &Values);
        if (!World || !GEngine) return;
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        World->SetGameInstance(NewObject<UGameInstance>(GEngine));
        World->GetWorldSettings()->DefaultGameMode = AAIHeroesGameMode::StaticClass();
        if (!World->SetGameMode(FURL())) return;
        Game = Cast<AAIHeroesGameMode>(World->GetAuthGameMode());
        Controller = World->SpawnActor<AAIHeroesPlayerController>();
        if (!Game || !Controller) return;
        // Explicitly initialize the production binding path once. No OS input.
        Controller->InputComponent = NewObject<UInputComponent>(Controller);
        Controller->InputComponent->RegisterComponent();
        Controller->SetupInputComponent();

        const TCHAR* Names[] = {TEXT("HQ"), TEXT("FOREST"), TEXT("ORE"),
            TEXT("OUTPOST"), TEXT("RUINS")};
        const EAIHeroesNodeType Types[] = {EAIHeroesNodeType::Headquarters,
            EAIHeroesNodeType::ForestMine, EAIHeroesNodeType::OreMine,
            EAIHeroesNodeType::Outpost, EAIHeroesNodeType::Ruins};
        for (int32 I = 0; I < 5; ++I)
        {
            const FVector Position = Game->GetHeroStrategicLocation() +
                FVector((I - 1) * 100.f, 0.f, 0.f);
            auto* Node = World->SpawnActor<AAIHeroesWorldNode>(Position, FRotator::ZeroRotator);
            if (!Node) return;
            Node->Configure(Types[I], Names[I], {});
            Node->SetOwned(I < 3);
            Game->Nodes.Add(Node);
            if (I == 0) HQ = Node;
            if (I == 1) Forest = Node;
            if (I == 3) Hostile = Node;
        }
        Game->SelectedNode = Forest;
        Game->FieldDwellings = FAIHeroesFieldDwellingRules::Initial();
        Game->PlayerArmy = {FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3),
            FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7)};
        Game->Gold = 1000;
        Game->Wood = 100;
        Game->Ore = 100;
    }

    ~FFieldInputWorld()
    {
        if (World)
        {
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }
    }

    bool Valid() const
    {
        return World && Game && Controller && Controller->InputComponent &&
            HQ && Forest && Hostile && Game->Nodes.Num() == 5;
    }

    int32 Press(const FKey Key) const
    {
        int32 Executed = 0;
        for (const FInputKeyBinding& Binding : Controller->InputComponent->KeyBindings)
        {
            if (Binding.Chord.Key == Key && Binding.KeyEvent == IE_Pressed &&
                !Binding.Chord.bShift && !Binding.Chord.bCtrl &&
                !Binding.Chord.bAlt && !Binding.Chord.bCmd)
            {
                Binding.KeyDelegate.Execute(Key);
                ++Executed;
            }
        }
        return Executed;
    }

    FString State() const
    {
        FString Result = FString::Printf(TEXT("%d/%d/%d/%d/%d/%d:%d/%d:%d"),
            Game->Gold, Game->Wood, Game->Ore, Game->Day, Game->MovesLeft,
            Game->FieldDwellings.Forest.bClaimed,
            Game->FieldDwellings.Forest.Available,
            Game->FieldDwellings.Ore.bClaimed, Game->FieldDwellings.Ore.Available);
        for (const auto& Stack : Game->PlayerArmy)
            Result += FString::Printf(TEXT("/%s:%d:%d:%d:%d:%d"), *Stack.UnitId.ToString(),
                Stack.TotalHealth, Stack.UnitHealth, Stack.Attack, Stack.Defense, Stack.Initiative);
        return Result;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesFieldInputBindingTest,
    "AIHeroes.InputBindings.CameraKeysDoNotIssueFieldOrders",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesFieldInputBindingTest::RunTest(const FString& Parameters)
{
    FFieldInputWorld F;
    if (!TestTrue(TEXT("Authoritative disposable controller world exists"), F.Valid())) return false;
    TestTrue(TEXT("Production field key contract is 3, separate from Q/E"),
        AAIHeroesPlayerController::FieldInteractionKey() == EKeys::Three);
    int32 FieldBindings = 0, CameraActionBindings = 0;
    for (const FInputKeyBinding& Binding : F.Controller->InputComponent->KeyBindings)
    {
        if (Binding.Chord.Key == EKeys::Three && Binding.KeyEvent == IE_Pressed) ++FieldBindings;
        if (Binding.Chord.Key == EKeys::Q || Binding.Chord.Key == EKeys::E) ++CameraActionBindings;
    }
    TestEqual(TEXT("Field action is bound to 3 exactly once in actual InputComponent"), FieldBindings, 1);
    TestEqual(TEXT("Q/E camera polling has no controller action delegates"), CameraActionBindings, 0);
    const FString Before = F.State();
    TestEqual(TEXT("Q does not dispatch a strategic order"), F.Press(EKeys::Q), 0);
    TestEqual(TEXT("E does not dispatch a strategic order"), F.Press(EKeys::E), 0);
    TestEqual(TEXT("Camera keys cannot claim, recruit or spend"), F.State(), Before);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesFieldInputDelegateTest,
    "AIHeroes.InputBindings.FieldKeyDispatchesClaimThenSinglePurchase",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesFieldInputDelegateTest::RunTest(const FString& Parameters)
{
    FFieldInputWorld F;
    if (!TestTrue(TEXT("Authoritative disposable controller world exists"), F.Valid())) return false;
    const int32 Gold = F.Game->Gold, Wood = F.Game->Wood, Ore = F.Game->Ore;
    const int32 Health = F.Game->PlayerArmy[0].TotalHealth;
    if (!TestEqual(TEXT("Actual 3 delegate is present once"), F.Press(EKeys::Three), 1)) return false;
    TestTrue(TEXT("First key action claims the eligible dwelling"), F.Game->FieldDwellings.Forest.bClaimed);
    TestEqual(TEXT("Claim grants no free soldiers"), F.Game->PlayerArmy[0].TotalHealth, Health);
    TestEqual(TEXT("Claim spends no gold"), F.Game->Gold, Gold);
    const auto Offer = FAIHeroesRecruitmentAdvisor::GetOffer(EAIHeroesRecruitChoice::Militia);
    TestEqual(TEXT("Second key action dispatches once"), F.Press(EKeys::Three), 1);
    TestEqual(TEXT("One pack adds its exact HP"), F.Game->PlayerArmy[0].TotalHealth, Health + Offer.Units.TotalHealth);
    TestEqual(TEXT("One pack charges exact gold"), F.Game->Gold, Gold - Offer.Gold);
    TestEqual(TEXT("One pack charges exact wood"), F.Game->Wood, Wood - Offer.Wood);
    TestEqual(TEXT("One pack charges exact ore"), F.Game->Ore, Ore - Offer.Ore);
    const FString AfterPurchase = F.State();
    F.Press(EKeys::Three);
    TestEqual(TEXT("Out-of-stock retry cannot charge or add troops"), F.State(), AfterPurchase);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesFieldInputModalTest,
    "AIHeroes.InputBindings.FieldKeyRespectsModalAndBattleLocks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesFieldInputModalTest::RunTest(const FString& Parameters)
{
    FFieldInputWorld F;
    if (!TestTrue(TEXT("Authoritative disposable controller world exists"), F.Valid())) return false;
    F.Game->ToggleAdvisorDossier();
    TestTrue(TEXT("Read-only dossier opened"), F.Game->IsAdvisorDossierVisible());
    FString Before = F.State();
    TestEqual(TEXT("Bound field delegate invoked while dossier open"), F.Press(EKeys::Three), 1);
    TestEqual(TEXT("Dossier prevents claim/purchase"), F.State(), Before);
    F.Game->CloseAdvisorDossier();

    F.Game->InteractWithNode(F.HQ);
    F.Game->ToggleBasePanel();
    TestTrue(TEXT("Base purchase panel opened"), F.Game->IsBasePanelVisible());
    F.Forest->SetActorLocation(F.Game->GetHeroStrategicLocation());
    F.Game->SelectedNode = F.Forest; // Eligible fixture even if the modal guard were absent.
    Before = F.State();
    F.Press(EKeys::Three);
    TestEqual(TEXT("Base panel prevents field order"), F.State(), Before);
    TestTrue(TEXT("Base panel guard, not a distance failure, refused the order"),
        F.Game->LastEvent.Contains(TEXT("BASE PANEL OPEN")));
    F.Game->CloseBasePanel();

    F.Hostile->Defenders = {FAIHeroesArmyStack(TEXT("Wolf"), 4, 9, 3, 1, 8)};
    F.Game->InteractWithNode(F.Hostile);
    if (!TestTrue(TEXT("Real live battle started"), F.Game->IsLiveBattleActive())) return false;
    Before = F.State();
    F.Press(EKeys::Three);
    TestEqual(TEXT("Running battle blocks field key"), F.State(), Before);
    F.Game->ToggleLiveBattlePause();
    F.Press(EKeys::Three);
    TestEqual(TEXT("Paused battle also blocks field key"), F.State(), Before);
    return true;
}
#endif
