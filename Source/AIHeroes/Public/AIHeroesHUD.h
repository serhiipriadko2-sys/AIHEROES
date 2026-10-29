#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AIHeroesHUD.generated.h"

class AAIHeroesGameMode;
class UTexture2D;

UCLASS()
class AIHEROES_API AAIHeroesHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
    int32 GetActionAt(const FVector2D& ScreenPoint) const;
    void PageAdvisorDossier(int32 Direction);
    bool CloseExplorerMenu();

private:
    UPROPERTY()
    TMap<FName,TObjectPtr<UTexture2D>> BattlePortraits;
    void DrawAmberBattle(const AAIHeroesGameMode& Rules);
    mutable bool bExplorerMenuOpen = false;
    FString ExplorerLastEvent;
    double ExplorerToastUntil = 0.0;
    void DrawExplorerHUD(const AAIHeroesGameMode& Rules);
    FVector2D CachedCanvasSize = FVector2D::ZeroVector;
    int32 CachedDossierPageCount = 1;
    float DossierFontScale = -1.f;
    float DossierGlyphWidth = 8.f;
    void GetActionRect(int32 Index, float& X, float& Y, float& W, float& H) const;
    void DrawAdvisorDossier(const AAIHeroesGameMode& Rules);
    void DrawBasePanel(const AAIHeroesGameMode& Rules);
};
