#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AIHeroesZonePreviewGameMode.generated.h"

class ACameraActor;

// Developer-only alternative GameMode for read-only generated-world preview.
// It is intentionally separate from the active campaign GameMode:
// no save slots, player army, combat authority or player actions exist here.
UCLASS()
class AIHEROES_API AAIHeroesZonePreviewGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AAIHeroesZonePreviewGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    // Optional command-line QA capture; never a player SaveGame write.
    UPROPERTY()
    TObjectPtr<ACameraActor> PreviewCamera;
    bool bCaptureEnabled = false;
    bool bCaptureRequested = false;
    bool bCameraLogged = false;
    float PreviewElapsed = 0.f;
};
