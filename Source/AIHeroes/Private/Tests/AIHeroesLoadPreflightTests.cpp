#include "AIHeroesLoadPreflight.h"
#include "AIHeroesCampaignSave.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

namespace
{
TArray<FAIHeroesActiveNodeIdentity> LoadTestActiveMap()
{
    return {
        {FName(TEXT("HQ")), EAIHeroesNodeType::Headquarters},
        {FName(TEXT("FOREST")), EAIHeroesNodeType::ForestMine},
        {FName(TEXT("ORE")), EAIHeroesNodeType::OreMine},
        {FName(TEXT("OUTPOST")), EAIHeroesNodeType::Outpost},
        {FName(TEXT("RUINS")), EAIHeroesNodeType::Ruins}
    };
}

TArray<FAIHeroesSavedNode> LoadTestSavedMap()
{
    TArray<FAIHeroesSavedNode> Saved;
    for (const auto& Identity : LoadTestActiveMap())
    {
        FAIHeroesSavedNode Node;
        Node.NodeId = Identity.Id;
        Node.Type = Identity.Type;
        Node.bOwned = Node.Type == EAIHeroesNodeType::Headquarters;
        Saved.Add(Node);
    }
    return Saved;
}

// Reproduce the previous GameMode identity checks, not an alternate loader.
bool PreviousLoadIdentityChecksAccept(const TArray<FAIHeroesActiveNodeIdentity>& Active,
    const TArray<FAIHeroesSavedNode>& Saved)
{
    if (Active.Num() != Saved.Num()) return false;
    for (const auto& Node : Active)
    {
        const auto* Match = Saved.FindByPredicate([&](const FAIHeroesSavedNode& Entry)
        {
            return Entry.NodeId == Node.Id;
        });
        if (!Match || Match->Type != Node.Type) return false;
    }
    return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesLoadMapIdentityTest,
    "AIHeroes.Campaign.LoadPreflight.MapIdentity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesLoadMapIdentityTest::RunTest(const FString& Parameters)
{
    auto Active = LoadTestActiveMap();
    const auto Saved = LoadTestSavedMap();
    FString Error;
    TestTrue(TEXT("Exact map accepted"), FAIHeroesLoadPreflight::Validate(
        Active, Saved, FName(TEXT("HQ")), FName(TEXT("ORE")), Error));
    Active.Swap(0, 4);
    Active.Swap(1, 3);
    TestTrue(TEXT("Actor order does not change identity matching"),
        FAIHeroesLoadPreflight::Validate(Active, Saved, FName(TEXT("HQ")), NAME_None, Error));
    TestTrue(TEXT("Success clears stale error"), Error.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesLoadDuplicateMapIdTest,
    "AIHeroes.Campaign.LoadPreflight.DuplicateActiveIdRegression",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesLoadDuplicateMapIdTest::RunTest(const FString& Parameters)
{
    auto Active = LoadTestActiveMap();
    const auto Saved = LoadTestSavedMap();
    // Five valid actors, but FOREST appears twice and ORE is absent.
    Active[2] = Active[1];
    TestTrue(TEXT("Fixture passes previous equal-count/per-node checks"),
        PreviousLoadIdentityChecksAccept(Active, Saved));
    FString Error;
    TestFalse(TEXT("Duplicate active ID cannot consume one saved node twice"),
        FAIHeroesLoadPreflight::Validate(Active, Saved, FName(TEXT("HQ")), NAME_None, Error));
    TestTrue(TEXT("Duplicate diagnostic retained"), Error.Contains(TEXT("duplicate")));
    Active[2].Id = FName(TEXT("forest"));
    TestFalse(TEXT("FName case alias is also a duplicate"),
        FAIHeroesLoadPreflight::Validate(Active, Saved, FName(TEXT("HQ")), NAME_None, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesLoadInvalidLayoutTest,
    "AIHeroes.Campaign.LoadPreflight.InvalidLayouts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesLoadInvalidLayoutTest::RunTest(const FString& Parameters)
{
    const auto ValidActive = LoadTestActiveMap();
    const auto ValidSaved = LoadTestSavedMap();
    FString Error;
    auto Reject = [&](const TCHAR* Label, const TArray<FAIHeroesActiveNodeIdentity>& Active,
        const TArray<FAIHeroesSavedNode>& Saved)
    {
        TestFalse(Label, FAIHeroesLoadPreflight::Validate(
            Active, Saved, FName(TEXT("HQ")), NAME_None, Error));
        TestFalse(TEXT("Rejection explains failure"), Error.IsEmpty());
    };

    auto Active = ValidActive;
    Active.RemoveAt(2);
    Reject(TEXT("Missing active node rejected"), Active, ValidSaved);
    Active = ValidActive;
    Active.Add({FName(TEXT("EXTRA")), EAIHeroesNodeType::Ruins});
    Reject(TEXT("Extra active node rejected"), Active, ValidSaved);
    Active = ValidActive;
    Active[2].Id = FName(TEXT("EXTRA"));
    Reject(TEXT("Equal count is not enough for exact coverage"), Active, ValidSaved);
    Active = ValidActive;
    Active[2].Id = NAME_None;
    Reject(TEXT("Empty active ID rejected"), Active, ValidSaved);
    Active = ValidActive;
    Active[2].Type = EAIHeroesNodeType::ForestMine;
    Reject(TEXT("Matching ID with different type rejected"), Active, ValidSaved);

    auto Saved = ValidSaved;
    Saved[2].NodeId = Saved[1].NodeId;
    Reject(TEXT("Duplicate saved ID rejected defensively"), ValidActive, Saved);
    Saved = ValidSaved;
    Saved[2].NodeId = NAME_None;
    Reject(TEXT("Empty saved ID rejected defensively"), ValidActive, Saved);
    Saved = ValidSaved;
    Active = ValidActive;
    Saved[2].Type = static_cast<EAIHeroesNodeType>(255);
    Active[2].Type = Saved[2].Type;
    Reject(TEXT("Matching unknown enum values are not valid node types"), Active, Saved);
    Reject(TEXT("Empty maps rejected"), {}, {});
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesLoadReferencesReadOnlyTest,
    "AIHeroes.Campaign.LoadPreflight.ReferencesReadOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesLoadReferencesReadOnlyTest::RunTest(const FString& Parameters)
{
    const auto Active = LoadTestActiveMap();
    auto Saved = LoadTestSavedMap();
    Saved[2].Defenders.Add(FAIHeroesArmyStack(TEXT("Bandit"), 6, 10, 4, 2, 4));
    FString Error;
    TestFalse(TEXT("Hero ID must exist"), FAIHeroesLoadPreflight::Validate(
        Active, Saved, FName(TEXT("MISSING")), NAME_None, Error));
    TestFalse(TEXT("Hero ID is required"), FAIHeroesLoadPreflight::Validate(
        Active, Saved, NAME_None, NAME_None, Error));
    TestFalse(TEXT("Explicit selected ID must exist"), FAIHeroesLoadPreflight::Validate(
        Active, Saved, FName(TEXT("HQ")), FName(TEXT("MISSING")), Error));
    TestEqual(TEXT("Rejection preserves saved guard health"), Saved[2].Defenders[0].TotalHealth, 60);
    TestFalse(TEXT("Rejection preserves saved ownership"), Saved[2].bOwned);
    TestEqual(TEXT("Rejection preserves map identities"), Active[2].Id, FName(TEXT("ORE")));
    TestTrue(TEXT("Optional absent selection remains valid"), FAIHeroesLoadPreflight::Validate(
        Active, Saved, FName(TEXT("HQ")), NAME_None, Error));
    TestTrue(TEXT("Successful retry clears prior diagnostics"), Error.IsEmpty());
    return true;
}
#endif
