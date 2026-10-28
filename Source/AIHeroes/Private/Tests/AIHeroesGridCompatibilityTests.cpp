#include "AIHeroesGridBattle.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesRecruitmentAdvisor.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesSplitGarrisonAdviceTest,
    "AIHeroes.Grid.SplitGarrisonAdviceCompatibility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesSplitGarrisonAdviceTest::RunTest(const FString& Parameters)
{
    FAIHeroesAdvisorSnapshot S;
    S.ProtocolTier = 1;
    S.MovesLeft = 4;
    S.Gold = 240; S.Wood = 35; S.Ore = 30;
    S.HeroPosition = FVector::ZeroVector;
    S.SelectedNodeId = FName(TEXT("OUTPOST"));
    S.PlayerArmy = {FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3),
        FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7)};
    FAIHeroesAdvisorNode Target;
    Target.Id = S.SelectedNodeId;
    Target.Type = EAIHeroesNodeType::Outpost;
    Target.Location = FVector(100.f, 0.f, 0.f);
    Target.Guards = {FAIHeroesArmyStack(TEXT("Raider"), 6, 11, 5, 2, 5),
        FAIHeroesArmyStack(TEXT("Raider"), 4, 11, 5, 2, 5)};
    S.Nodes.Add(Target);
    const auto Advice = FAIHeroesRecruitmentAdvisor::Evaluate(S);
    TestTrue(TEXT("Same UnitId in two enemy stacks is valid Grid V2 input"), Advice.bValidInput);
    TestTrue(TEXT("Current army forecast remains available for split garrisons"),
        Advice.Options.Num() == 3 && Advice.Options[0].bForecastAvailable);

    // Three ordinary recruitment purchases of each type produce this army;
    // verify the new outpost is beatable through normal non-neural progression.
    const auto Progressed = FAIHeroesGridBattle::RunToEnd(
        {FAIHeroesArmyStack(TEXT("Militia"), 23, 12, 4, 2, 3),
         FAIHeroesArmyStack(TEXT("Scout"), 9, 8, 6, 1, 7)}, Target.Guards, 1);
    TestTrue(TEXT("Recruitment progression can conquer the two-group outpost"), Progressed.bVictory);
    return true;
}
#endif
