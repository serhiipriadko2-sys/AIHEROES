#include "AIHeroesZoneExpeditionArt.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/StaticMesh.h"
#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"

// All three optional models must be present or all absent. This test does not
// require the editor art kit to be installed: the fallback is intentional.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesArchitectureV6OptionalKitTest,
    "AIHeroes.Visual.ArchitectureV6.OptionalKitIsCoherent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesArchitectureV6OptionalKitTest::RunTest(const FString&)
{
    const TCHAR* Names[] = {
        TEXT("SM_CitadelCrownV6"),
        TEXT("SM_FrontierGateV6"),
        TEXT("SM_ArchiveSanctumV6"),
    };
    int32 Imported = 0;
    for (const TCHAR* Name : Names)
    {
        const FString PackageName = FString::Printf(
            TEXT("/Game/AmberMarch/Meshes/%s"), Name);
        if (!FPackageName::DoesPackageExist(PackageName)) continue;
        UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr,
            *FString::Printf(TEXT("/Game/AmberMarch/Meshes/%s.%s"),
                Name, Name));
        if (!TestNotNull(TEXT("Present V6 asset loads correctly"), Mesh))
            return false;
        ++Imported;
        const FVector Extents = Mesh->GetBounds().BoxExtent;
        TestTrue(TEXT("Authored architectural mesh is not an empty placeholder"),
            Extents.GetMax() > 100.f && Extents.GetMax() < 1300.f);
        TestTrue(TEXT("Original building has several palette materials"),
            Mesh->GetStaticMaterials().Num() >= 3);
    }
    TestTrue(TEXT("Optional pack never partially replaces the existing map"),
        Imported == 0 || Imported == 3);
    if (Imported == 0)
        AddInfo(TEXT("V6 kit not yet imported; existing AmberMarch fallbacks remain authoritative"));
    else
        AddInfo(TEXT("All three V6 meshes loaded from the same additive /Game/AmberMarch/Meshes kit"));
    return true;
}
#endif
