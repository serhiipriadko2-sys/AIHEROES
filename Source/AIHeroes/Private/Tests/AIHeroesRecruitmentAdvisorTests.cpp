#include "AIHeroesRecruitmentAdvisor.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

namespace
{
FAIHeroesAdvisorSnapshot RecruitmentScenario()
{
    FAIHeroesAdvisorSnapshot S;
    S.ProtocolTier = 1;
    S.MovesLeft = 4;
    S.Gold = 240;
    S.Wood = 35;
    S.Ore = 30;
    S.HeroPosition = FVector::ZeroVector;
    S.SelectedNodeId = FName(TEXT("TARGET"));
    FAIHeroesAdvisorNode N;
    N.Id = S.SelectedNodeId;
    N.Location = FVector(100.f, 0.f, 0.f);
    N.Type = EAIHeroesNodeType::Outpost;
    N.Guards.Add(FAIHeroesArmyStack(TEXT("Guardian"), 1, 24, 7, 4, 2));
    S.Nodes.Add(N);
    return S;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHRecruitProgression,
    "AIHeroes.Recruitment.ProtocolGateAndCatalog",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHRecruitProgression::RunTest(const FString& Parameters)
{
    FAIHeroesAdvisorSnapshot S = RecruitmentScenario();
    S.ProtocolTier = 0;
    const FAIHeroesRecruitAdvice Seed = FAIHeroesRecruitmentAdvisor::Evaluate(S);
    TestTrue(TEXT("Seed is valid"), Seed.bValidInput);
    TestEqual(TEXT("All three alternatives visible"), Seed.Options.Num(), 3);
    TestEqual(TEXT("No hidden ranking before protocol"), Seed.RecommendedIndex, INDEX_NONE);
    for (const FAIHeroesRecruitOption& O : Seed.Options)
        TestFalse(TEXT("Seed never simulates a battle"), O.bForecastAvailable);
    TestTrue(TEXT("Explains locked ability"), Seed.Summary.Contains(TEXT("LOCKED")));
    const auto Militia = FAIHeroesRecruitmentAdvisor::GetOffer(EAIHeroesRecruitChoice::Militia);
    const auto Scout = FAIHeroesRecruitmentAdvisor::GetOffer(EAIHeroesRecruitChoice::Scouts);
    TestEqual(TEXT("Militia count"), Militia.Units.Count(), 5);
    TestEqual(TEXT("Militia price"), Militia.Gold, 50);
    TestEqual(TEXT("Militia material"), Militia.Wood, 5);
    TestEqual(TEXT("Scout count"), Scout.Units.Count(), 2);
    TestEqual(TEXT("Scout price"), Scout.Gold, 70);
    TestEqual(TEXT("Scout material"), Scout.Ore, 4);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHRecruitChoice,
    "AIHeroes.Recruitment.ForecastSelectsUsefulUnit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHRecruitChoice::RunTest(const FString& Parameters)
{
    auto S = RecruitmentScenario();
    const auto R = FAIHeroesRecruitmentAdvisor::Evaluate(S);
    TestTrue(TEXT("Reachable target"), R.bTargetReachable);
    TestEqual(TEXT("Ranged scouts reduce losses against a guardian"), R.RecommendedIndex, 2);
    TestTrue(TEXT("Scouts predict victory"), R.Options[2].bVictory);
    TestTrue(TEXT("Scouts lose less HP than militia"), R.Options[2].LostHP < R.Options[1].LostHP);
    TestTrue(TEXT("No automatic purchase"), R.Summary.Contains(TEXT("No order issued")));
    TestTrue(TEXT("Scoring is disclosed"), R.Detail.Contains(TEXT("Score =")));
    S.PlayerArmy.Add(FAIHeroesArmyStack(TEXT("Scout"), 5, 8, 6, 1, 7));
    const auto Strong = FAIHeroesRecruitmentAdvisor::Evaluate(S);
    TestEqual(TEXT("Do not sell unnecessary reinforcements"), Strong.RecommendedIndex, 0);
    TestTrue(TEXT("Current army wins"), Strong.Options[0].bVictory);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHRecruitBudget,
    "AIHeroes.Recruitment.BudgetAndNoWinningOption",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHRecruitBudget::RunTest(const FString& Parameters)
{
    auto S = RecruitmentScenario();
    S.Gold = 50;
    const auto Cheap = FAIHeroesRecruitmentAdvisor::Evaluate(S);
    TestEqual(TEXT("Only affordable winning purchase selected"), Cheap.RecommendedIndex, 1);
    TestFalse(TEXT("Scouts unaffordable"), Cheap.Options[2].bAffordable);
    TestFalse(TEXT("Unaffordable option not simulated"), Cheap.Options[2].bForecastAvailable);
    S.Wood = 0;
    const auto Poor = FAIHeroesRecruitmentAdvisor::Evaluate(S);
    TestEqual(TEXT("Cannot promise victory without resources"), Poor.RecommendedIndex, INDEX_NONE);
    TestFalse(TEXT("Insufficient wood locks militia"), Poor.Options[1].bAffordable);
    S = RecruitmentScenario();
    S.Nodes[0].Guards = {FAIHeroesArmyStack(TEXT("Guardian"), 30, 24, 7, 4, 2)};
    const auto Dangerous = FAIHeroesRecruitmentAdvisor::Evaluate(S);
    TestEqual(TEXT("No losing option recommended as winning"), Dangerous.RecommendedIndex, INDEX_NONE);
    TestTrue(TEXT("Clear avoid advice"), Dangerous.Summary.Contains(TEXT("Avoid")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHRecruitTarget,
    "AIHeroes.Recruitment.SelectionOwnershipAndMovement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHRecruitTarget::RunTest(const FString& Parameters)
{
    auto S = RecruitmentScenario();
    S.SelectedNodeId = NAME_None;
    const auto None = FAIHeroesRecruitmentAdvisor::Evaluate(S);
    TestEqual(TEXT("No guessed target"), None.RecommendedIndex, INDEX_NONE);
    S.SelectedNodeId = FName(TEXT("MISSING"));
    TestEqual(TEXT("Stale selection gives no recommendation"),
        FAIHeroesRecruitmentAdvisor::Evaluate(S).RecommendedIndex, INDEX_NONE);
    S = RecruitmentScenario();
    S.Nodes[0].bOwned = true;
    TestEqual(TEXT("Owned node requires no reinforcements"),
        FAIHeroesRecruitmentAdvisor::Evaluate(S).RecommendedIndex, 0);
    S.Nodes[0].bOwned = false;
    S.Nodes[0].Guards.Empty();
    TestEqual(TEXT("Empty node requires no reinforcements"),
        FAIHeroesRecruitmentAdvisor::Evaluate(S).RecommendedIndex, 0);
    S = RecruitmentScenario();
    S.MovesLeft = 0;
    const auto Tired = FAIHeroesRecruitmentAdvisor::Evaluate(S);
    TestFalse(TEXT("No false reachable attack"), Tired.bTargetReachable);
    TestEqual(TEXT("No imminent attack recommendation"), Tired.RecommendedIndex, INDEX_NONE);
    for (const auto& O : Tired.Options)
        TestFalse(TEXT("Unreachable battle not simulated"), O.bForecastAvailable);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHRecruitValidation,
    "AIHeroes.Recruitment.RejectsMalformedSnapshot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHRecruitValidation::RunTest(const FString& Parameters)
{
    auto S = RecruitmentScenario();
    S.Gold = -1;
    TestFalse(TEXT("Negative budget rejected"), FAIHeroesRecruitmentAdvisor::Evaluate(S).bValidInput);
    S = RecruitmentScenario(); S.ProtocolTier = 4;
    TestFalse(TEXT("Unknown protocol rejected"), FAIHeroesRecruitmentAdvisor::Evaluate(S).bValidInput);
    S = RecruitmentScenario(); S.MovesLeft = 5;
    TestFalse(TEXT("Impossible movement rejected"), FAIHeroesRecruitmentAdvisor::Evaluate(S).bValidInput);
    S = RecruitmentScenario();
    S.PlayerArmy.Add(FAIHeroesArmyStack(TEXT("ForgedUnit"), 5, 10, 10, 1, 1));
    TestFalse(TEXT("Unknown friendly unit rejected"), FAIHeroesRecruitmentAdvisor::Evaluate(S).bValidInput);
    S = RecruitmentScenario(); S.Nodes[0].Guards[0].Attack = MAX_int32;
    TestFalse(TEXT("Unbounded enemy stats rejected"), FAIHeroesRecruitmentAdvisor::Evaluate(S).bValidInput);
    S = RecruitmentScenario();
    S.PlayerArmy = {FAIHeroesArmyStack(TEXT("Militia"), 5, 12, 4, 2, 3)};
    S.PlayerArmy[0].TotalHealth = 500000;
    TestFalse(TEXT("Purchase cannot overflow campaign stack cap"),
        FAIHeroesRecruitmentAdvisor::Evaluate(S).Options[1].bAffordable);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHRecruitPurity,
    "AIHeroes.Recruitment.ReadOnlyAndDeterministic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHRecruitPurity::RunTest(const FString& Parameters)
{
    const auto S = RecruitmentScenario();
    const auto A = FAIHeroesRecruitmentAdvisor::Evaluate(S);
    const auto B = FAIHeroesRecruitmentAdvisor::Evaluate(S);
    TestEqual(TEXT("Same result"), A.Summary, B.Summary);
    TestEqual(TEXT("Same evidence"), A.Detail, B.Detail);
    TestEqual(TEXT("Gold unchanged"), S.Gold, 240);
    TestEqual(TEXT("Wood unchanged"), S.Wood, 35);
    TestEqual(TEXT("Ore unchanged"), S.Ore, 30);
    TestEqual(TEXT("Movement unchanged"), S.MovesLeft, 4);
    TestTrue(TEXT("No units purchased"), S.PlayerArmy.IsEmpty());
    TestFalse(TEXT("No node captured"), S.Nodes[0].bOwned);
    TestEqual(TEXT("Defenders untouched"), S.Nodes[0].Guards[0].TotalHealth, 24);
    TestEqual(TEXT("No protocol unlock"), S.ProtocolTier, 1);
    return true;
}
#endif
