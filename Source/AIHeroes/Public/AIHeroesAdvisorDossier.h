#pragma once

#include "CoreMinimal.h"
#include "AIHeroesHUDLayout.h"

// Transient UI evidence only. Never serialized as campaign state or AI memory.
struct FAIHeroesAdvisorDossier
{
    FString Title;
    TArray<FString> Paragraphs;
    FString EvidenceKey;
    int32 ObservedDay = 0;
    bool bRecruitment = false;

    bool HasContent() const { return !Title.IsEmpty() && !Paragraphs.IsEmpty(); }
};

struct FAIHeroesDossierLayout
{
    FAIHeroesHUDActionRect Panel;
    float TextX = 0.f;
    float TextY = 0.f;
    float TextWidth = 0.f;
    float RowHeight = 22.f;
    int32 RowsPerPage = 1;
};

// Pure display helpers. No actors, simulation, purchases, files or network.
class AIHEROES_API FAIHeroesAdvisorDossierRules
{
public:
    static constexpr int32 ToggleAction = 210;
    static constexpr int32 PreviousAction = 211;
    static constexpr int32 NextAction = 212;
    static constexpr int32 RefreshAction = 213;
    static constexpr int32 CloseAction = 214;
    static constexpr int32 ConsumeAction = 219;
    static constexpr int32 MaxCharacters = 12000;

    // All generated text is reachable through pages, including long tokens.
    // Oversized input gets an explicit limit marker, never silent truncation.
    static TArray<FString> Wrap(const TArray<FString>& Paragraphs, int32 Columns);
    static int32 PageCount(int32 LineCount, int32 RowsPerPage);
    static int32 ClampPage(int32 Page, int32 Count);
    static FAIHeroesDossierLayout Layout(const FVector2D& Viewport);
    static FAIHeroesHUDActionRect Button(int32 ActionId, const FVector2D& Viewport);
    static FAIHeroesHUDActionRect OpenButton(const FVector2D& Viewport);
    // A modal click not on a button is consumed, never sent to the map below.
    static int32 HitTest(const FVector2D& Point, const FVector2D& Viewport);
};
