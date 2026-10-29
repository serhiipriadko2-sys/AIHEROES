#include "AIHeroesZoneExpeditionArt.h"
#include "Engine/StaticMesh.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesExpeditionArtSeedTest,
    "AIHeroes.Visual.ExpeditionArt.SeedOnlyLandscape",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesExpeditionArtSeedTest::RunTest(const FString&)
{
    const auto A = FAIHeroesZoneExpeditionArt::Scatter(20261029u, 94, 0x3a1915u);
    const auto B = FAIHeroesZoneExpeditionArt::Scatter(20261029u, 94, 0x3a1915u);
    const auto C = FAIHeroesZoneExpeditionArt::Scatter(20261030u, 94, 0x3a1915u);
    if (!TestTrue(TEXT("Same seed yields identical cosmetic positions"), A == B))
        return false;
    TestTrue(TEXT("Different seed visibly changes grove placement"), A != C);
    TestTrue(TEXT("Forest instance count is bounded"), A.Num() > 20 && A.Num() <= 94);
    TestTrue(TEXT("A deliberately overlarge request fails closed"),
        FAIHeroesZoneExpeditionArt::Scatter(0u, 181, 1u).IsEmpty());
    TestTrue(TEXT("Negative request fails closed"),
        FAIHeroesZoneExpeditionArt::Scatter(0u, -1, 1u).IsEmpty());
    for (int32 I = 0; I < A.Num(); ++I)
    {
        TestTrue(TEXT("Each visual candidate stays on the authored island"),
            FMath::Abs(A[I].X) <= 1470.f && FMath::Abs(A[I].Y) <= 1440.f);
        for (int32 J = I + 1; J < A.Num(); ++J)
            if (!TestTrue(TEXT("Scenery avoids duplicate positions"),
                FVector2D::DistSquared(A[I], A[J]) >= 125.f * 125.f))
                return false;
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesExpeditionArtContentTest,
    "AIHeroes.Visual.ExpeditionArt.OriginalKitAvailable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesExpeditionArtContentTest::RunTest(const FString&)
{
    const TCHAR* Assets[] = { TEXT("SM_Island"), TEXT("SM_Pine"),
        TEXT("SM_Boulder"), TEXT("SM_Seeker"), TEXT("SM_Hearth"),
        TEXT("SM_Sawmill"), TEXT("SM_Quarry"), TEXT("SM_Watchtower"),
        TEXT("SM_Archive") };
    for (const TCHAR* Name : Assets)
    {
        auto* Mesh = LoadObject<UStaticMesh>(nullptr,
            *FString::Printf(TEXT("/Game/AmberMarch/Meshes/%s.%s"), Name, Name));
        if (!TestNotNull(FString::Printf(TEXT("Imported original mesh %s"), Name), Mesh))
            return false;
        TestTrue(TEXT("Mesh has nonzero geometric extent"),
            Mesh->GetBounds().SphereRadius > 5.f &&
            Mesh->GetBounds().SphereRadius < 3000.f);
    }
    return true;
}
#endif
