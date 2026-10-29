#include "AIHeroesHUD.h"
#include "AIHeroesHUDLayout.h"
#include "AIHeroesHUDTextLayout.h"
#include "AIHeroesMapLabelLayout.h"
#include "AIHeroesBasePanel.h"

#include "AIHeroesAdvisorDossier.h"
#include "AIHeroesGameMode.h"
#include "AIHeroesWorldNode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
const TCHAR* ActionLabels[8] = {
    TEXT("CONFIRM MOVE / FIGHT"),
    TEXT("ASK HERO"),
    TEXT("NEXT DAY"),
    TEXT("RECRUIT MILITIA"),
    TEXT("RECRUIT SCOUTS"),
    TEXT("UNLOCK AI PROTOCOL"),
    TEXT("MODEL SLOT +"),
    TEXT("PLAN ROUTE Z")
};
const TCHAR* UtilityLabels[5] = {
    TEXT("SAVE K"), TEXT("LOAD L"), TEXT("SLOT C"), TEXT("REPLAY J"), TEXT("3D REPLAY V")
};
}

void AAIHeroesHUD::GetActionRect(
    int32 Index, float& X, float& Y, float& W, float& H) const
{
    const AAIHeroesGameMode* Rules =
        Cast<AAIHeroesGameMode>(UGameplayStatics::GetGameMode(this));
    const FAIHeroesHUDActionRect Rect = FAIHeroesHUDLayout::Rect(
        Index, CachedCanvasSize, Rules && Rules->IsLiveBattleActive());
    X = Rect.X;
    Y = Rect.Y;
    W = Rect.W;
    H = Rect.H;
}

int32 AAIHeroesHUD::GetActionAt(const FVector2D& ScreenPoint) const
{
    const AAIHeroesGameMode* Rules =
        Cast<AAIHeroesGameMode>(UGameplayStatics::GetGameMode(this));
    if (Rules && Rules->IsLiveBattleActive())
    {
        return FAIHeroesHUDLayout::HitTest(ScreenPoint, CachedCanvasSize, true);
    }
    if (Rules && Rules->IsBasePanelVisible())
        return FAIHeroesBasePanelRules::HitTest(ScreenPoint, CachedCanvasSize);
    if (Rules && Rules->IsAdvisorDossierVisible())
    {
        // Even a click outside the panel is consumed by this read-only modal.
        return FAIHeroesAdvisorDossierRules::HitTest(ScreenPoint, CachedCanvasSize);
    }
    if (Rules && !Rules->IsBattleReplayActive() &&
        FAIHeroesAdvisorDossierRules::OpenButton(CachedCanvasSize).Contains(ScreenPoint))
    {
        return FAIHeroesAdvisorDossierRules::ToggleAction;
    }
    if (Rules && !Rules->IsBattleReplayActive() &&
        FAIHeroesBasePanelRules::OpenButton(CachedCanvasSize).Contains(ScreenPoint))
        return FAIHeroesBasePanelRules::ToggleAction;
    return FAIHeroesHUDLayout::HitTest(ScreenPoint, CachedCanvasSize, false);
}
void AAIHeroesHUD::DrawHUD()
{
    Super::DrawHUD();
    const AAIHeroesGameMode* Rules =
        Cast<AAIHeroesGameMode>(UGameplayStatics::GetGameMode(this));
    if (!Rules || !Canvas || !GEngine)
    {
        return;
    }

    CachedCanvasSize = FVector2D(Canvas->SizeX, Canvas->SizeY);
    const float Width = Canvas->SizeX;
    const float Height = Canvas->SizeY;
    const int32 Columns = Width >= 1150.f ? 4 : (Width >= 800.f ? 3 : 2);
    const float BottomHeight = FAIHeroesHUDLayout::BottomPanelHeight(Width);
    const float Scale = Width < 700.f ? 0.78f : 1.f;
    const int32 TextLimit = FMath::Clamp(FMath::FloorToInt(Width / 8.f), 40, 152);
    UFont* Font = GEngine->GetSmallFont();
    auto DrawFitted = [this, Font](const FString& Text, const FLinearColor& Color,
        float X, float Y, float MaxWidth, float FontScale)
    {
        auto Measure = [this, Font, FontScale](const FString& Value)
        {
            float W = 0.f, H = 0.f;
            GetTextSize(Value, W, H, Font, FontScale);
            return W;
        };
        const FString Display = FAIHeroesHUDTextLayout::FitLine(Text, MaxWidth, Measure);
        if (!Display.IsEmpty()) DrawText(Display, Color, X, Y, Font, FontScale);
    };
    auto DrawHeader = [this, Font, &DrawFitted](const FString& Text,
        const FLinearColor& Color, float Y, float FontScale)
    {
        float W = 0.f, H = 0.f;
        GetTextSize(TEXT("Ag"), W, H, Font, FontScale);
        const float Span = FAIHeroesHUDTextLayout::HeaderLineWidth(CachedCanvasSize, Y, FMath::Max(1.f,H));
        DrawFitted(Text, Color, 18.f, Y, Span, FontScale);
    };
    if (Rules->IsBasePanelVisible())
    {
        DrawBasePanel(*Rules);
        return;
    }

    if (Rules->IsLiveBattleActive())
    {
        DrawRect(FLinearColor(0.015f, 0.025f, 0.045f, 0.96f), 0.f, 0.f, Width, 143.f);
        DrawText(FString::Printf(TEXT("AI HEROES | LIVE GRID V2 | PROTOCOL %d | HERO COMMANDS"), Rules->ProtocolTier),
            FLinearColor(0.4f, 0.9f, 1.f), 16.f, 12.f, Font, Scale * 1.15f);
        DrawText(Rules->GetLiveBattleStatus().Left(TextLimit), FLinearColor::White,
            16.f, 42.f, Font, Scale);
        DrawText(TEXT("HERO: ") + Rules->GetLiveArmyStatus(true).Left(TextLimit - 6),
            FLinearColor(0.4f, 0.9f, 1.f), 16.f, 69.f, Font, Scale);
        DrawText(TEXT("ENEMY: ") + Rules->GetLiveArmyStatus(false).Left(TextLimit - 7),
            FLinearColor(1.f, 0.65f, 0.3f), 16.f, 95.f, Font, Scale);
        DrawText(TEXT("Q/E orbit  R/F zoom | SPACE pause | T speed | B fast finish | ENTER return after result"),
            FLinearColor(0.8f, 0.85f, 0.9f), 16.f, 121.f, Font, Scale * 0.85f);
        DrawRect(FLinearColor(0.015f, 0.025f, 0.045f, 0.96f),
            0.f, Height - 126.f, Width, 126.f);
        DrawText(Rules->LastEvent.Left(TextLimit), FLinearColor::White,
            16.f, Height - 115.f, Font, Scale);
        const bool bResult = Rules->GetLiveBattlePhase() == EAIHeroesLiveBattlePhase::Result;
        const FString Labels[] = {
            Rules->GetLiveBattlePhase() == EAIHeroesLiveBattlePhase::Paused ? TEXT("RESUME") : TEXT("PAUSE"),
            FString::Printf(TEXT("SPEED %dx"), FMath::RoundToInt(Rules->GetLiveBattleSpeed())),
            TEXT("FAST FINISH"), bResult ? TEXT("RETURN TO MAP") : TEXT("RESULT PENDING")};
        for (int32 I = 0; I < 4; ++I)
        {
            float X, Y, W, H;
            GetActionRect(100 + I, X, Y, W, H);
            const bool bEnabled = I == 3 ? bResult : !bResult;
            DrawRect(bEnabled ? FLinearColor(0.04f, 0.28f, 0.34f, 1.f) :
                FLinearColor(0.08f, 0.1f, 0.13f, 1.f), X, Y, W, H);
            DrawText(Labels[I], bEnabled ? FLinearColor::White : FLinearColor(0.4f, 0.4f, 0.4f),
                X + 9.f, Y + 14.f, Font, Scale);
        }
        return; // No strategic node labels or hit targets under the arena.
    }

    const auto MapContent = FAIHeroesMapLabelLayout::MapBounds(CachedCanvasSize);
    TArray<FAIHeroesHUDActionRect> MapLabelRects;
    // Labels are display-only. Unknown/hidden nodes never enter the packer.
    if (const APlayerController* PC = GetWorld()->GetFirstPlayerController();
        PC && !Rules->IsBattleReplayActive() && MapContent.W > 0.f)
    {
        TArray<FAIHeroesMapLabelRequest> Requests;
        for (const AAIHeroesWorldNode* Node : Rules->Nodes)
        {
            if (!IsValid(Node) || Node->IsHidden() ||
                !Rules->IsNodeDiscovered(FName(*Node->GetDisplayName()))) continue;
            FAIHeroesMapLabelRequest R;
            R.Id = FName(*Node->GetDisplayName());
            if (!PC->ProjectWorldLocationToScreen(
                Node->GetActorLocation() + FVector(0.f, 0.f, 260.f), R.Anchor)) continue;
            R.bKnown = true;
            R.bSelected = Rules->SelectedNode.Get() == Node;
            R.bHasDetail = !Rules->GetFieldDwellingLabel(Node->NodeType).IsEmpty();
            Requests.Add(R);
        }
        for (const auto& Placed : FAIHeroesMapLabelLayout::Place(CachedCanvasSize, Requests))
        {
            const AAIHeroesWorldNode* Node = nullptr;
            for (const AAIHeroesWorldNode* Candidate : Rules->Nodes)
                if (IsValid(Candidate) && FName(*Candidate->GetDisplayName()) == Placed.Id)
                { Node = Candidate; break; }
            if (!Node) continue;
            MapLabelRects.Add(Placed.Rect);
            const float LabelX = Placed.Rect.X, LabelY = Placed.Rect.Y;
            const bool bSelected = Rules->SelectedNode.Get() == Node;
            const FLinearColor Color = bSelected
                ? FLinearColor(0.3f, 1.f, 1.f)
                : Node->bOwned ? FLinearColor(0.3f, 1.f, 0.55f)
                : Node->Defenders.IsEmpty() ? FLinearColor(1.f, 0.92f, 0.33f)
                : FLinearColor(1.f, 0.45f, 0.31f);
            // A clipped leader preserves association after a label is displaced.
            const FVector2D Anchor(
                FMath::Clamp(static_cast<float>(Placed.Anchor.X), MapContent.X, MapContent.X + MapContent.W),
                FMath::Clamp(static_cast<float>(Placed.Anchor.Y), MapContent.Y, MapContent.Y + MapContent.H));
            const FVector2D Edge(
                FMath::Clamp(static_cast<float>(Anchor.X), LabelX, LabelX + Placed.Rect.W),
                FMath::Clamp(static_cast<float>(Anchor.Y), LabelY, LabelY + Placed.Rect.H));
            if (FVector2D::DistSquared(Anchor, Edge) > 16.0)
                DrawLine(Anchor.X, Anchor.Y, Edge.X, Edge.Y, Color, 1.f);
            DrawRect(bSelected ? FLinearColor(0.02f, 0.16f, 0.21f, 1.f) :
                FLinearColor(0.012f, 0.027f, 0.045f, 1.f), LabelX, LabelY, 150.f, 28.f);
            DrawText(FString::Printf(TEXT("%s%s%s"), bSelected ? TEXT("> ") : TEXT(""),
                *Node->GetDisplayName(), Node->bOwned ? TEXT(" [OURS]") : TEXT("")),
                Color, LabelX + 9.f, LabelY + 6.f, Font, Scale);
            if (Placed.bShowDetail)
            {
                const FString Field = Rules->GetFieldDwellingLabel(Node->NodeType);
                DrawRect(FLinearColor(0.12f, 0.12f, 0.045f, 1.f), LabelX, LabelY + 28.f, 150.f, 24.f);
                DrawText(Field.Left(23), FLinearColor(0.94f, 0.97f, 0.63f),
                    LabelX + 5.f, LabelY + 32.f, Font, Scale * 0.84f);
            }
        }
    }

    // A route is a short-lived, READ-ONLY recommendation. HUD never selects
    // or moves a pawn. Lines are rendered only when both endpoints project
    // onto the unobstructed 3D map (not beneath HUD controls).
    const FAIHeroesRoutePlan& Route = Rules->GetRoutePreview();
    if (Route.IsValid() && !Rules->IsBattleReplayActive() && MapContent.W > 0.f)
    {
        if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
        {
            const float MapTop = MapContent.Y;
            const float MapBottom = MapContent.Y + MapContent.H;
            auto OnMap = [MapContent](const FVector2D& P) { return MapContent.Contains(P); };

            FVector2D Previous;
            bool bPreviousOnMap = PC->ProjectWorldLocationToScreen(
                Rules->GetHeroStrategicLocation() + FVector(0.f, 0.f, 290.f), Previous) &&
                OnMap(Previous);

            for (int32 StopIndex = 0; StopIndex < Route.Stops.Num(); ++StopIndex)
            {
                const AAIHeroesWorldNode* StopNode = nullptr;
                for (const AAIHeroesWorldNode* Candidate : Rules->Nodes)
                {
                    if (IsValid(Candidate) &&
                        Rules->IsNodeDiscovered(FName(*Candidate->GetDisplayName())) &&
                        FName(*Candidate->GetDisplayName()) == Route.Stops[StopIndex])
                    {
                        StopNode = Candidate;
                        break;
                    }
                }
                if (!IsValid(StopNode))
                {
                    bPreviousOnMap = false;
                    continue;
                }

                FVector2D At;
                const bool bOnMap = PC->ProjectWorldLocationToScreen(
                    StopNode->GetActorLocation() + FVector(0.f, 0.f, 290.f), At) &&
                    OnMap(At);
                const FLinearColor Color = StopIndex == 0
                    ? FLinearColor(0.24f, 0.94f, 0.89f, 1.f)
                    : FLinearColor(1.f, 0.78f, 0.29f, 1.f);
                if (bPreviousOnMap && bOnMap && Rules->TravelRuleVersion == 1)
                {
                    DrawLine(Previous.X, Previous.Y, At.X, At.Y, Color, 3.f);
                }
                if (bOnMap)
                {
                    const float TagX = FMath::Clamp(At.X - 38.f, 6.f, FMath::Max(6.f, Width - 83.f));
                    const float TagY = FMath::Clamp(At.Y - 63.f, MapTop, FMath::Max(MapTop, MapBottom - 24.f));
                    FAIHeroesHUDActionRect Tag;
                    Tag.X = TagX; Tag.Y = TagY; Tag.W = 77.f; Tag.H = 23.f;
                    bool bFree = FAIHeroesMapLabelLayout::ContainsRect(MapContent, Tag);
                    for (const auto& Label : MapLabelRects)
                        bFree &= !FAIHeroesMapLabelLayout::Overlaps(Label, Tag);
                    if (bFree)
                    {
                        MapLabelRects.Add(Tag);
                        DrawRect(FLinearColor(0.02f, 0.13f, 0.19f, 1.f), TagX, TagY, 77.f, 23.f);
                        DrawText(FString::Printf(TEXT("PLAN %d"), StopIndex + 1),
                            Color, TagX + 7.f, TagY + 4.f, Font, Scale);
                    }
                }
                Previous = At;
                bPreviousOnMap = bOnMap;
            }
        }
    }

    DrawRect(FLinearColor(0.015f, 0.022f, 0.04f, 0.92f),
        0.f, 0.f, Width, 175.f);
    const bool bCompactHeader = Width < 1050.f;
    DrawHeader(bCompactHeader
        ? FString::Printf(TEXT("AI HEROES | TRAVEL V%d"), Rules->TravelRuleVersion)
        : FString::Printf(TEXT("AI HEROES | TRAVEL V%d | PLAYER DECIDES / AI FIGHTS"), Rules->TravelRuleVersion),
        FLinearColor(0.42f, 0.88f, 1.f), 12.f, Scale * 1.3f);
    DrawHeader(bCompactHeader
        ? FString::Printf(TEXT("DAY %d | MOVE %d/4 | GOLD %d W %d O %d DATA %d"),
            Rules->Day, Rules->MovesLeft, Rules->Gold, Rules->Wood, Rules->Ore, Rules->DataFragments)
        : FString::Printf(TEXT("DAY %d  |  MOVES %d/4  |  GOLD %d  |  WOOD %d  |  ORE %d  |  DATA %d"),
            Rules->Day, Rules->MovesLeft, Rules->Gold, Rules->Wood, Rules->Ore, Rules->DataFragments),
        FLinearColor::White, 42.f, Scale);
    DrawHeader(TEXT("ARMY: ") + Rules->GetArmySummary() + FString::Printf(
        TEXT(" | HQ L%d | +%d GOLD/DAY"), Rules->BaseTier, Rules->GetDailyGoldIncome()),
        FLinearColor(0.88f, 0.95f, 0.81f), 68.f, Scale);
    DrawHeader(TEXT("AI: ") + Rules->GetCognitionSummary() + TEXT(" | ") + Rules->GetScoutingStatus(),
        FLinearColor(1.f, 0.85f, 0.35f), 94.f, Scale);
    DrawHeader(Rules->GetSelectionSummary(), FLinearColor(0.4f, 1.f, 0.92f), 119.f, Scale);
    DrawHeader(bCompactHeader
        ? TEXT("CLICK select | ENTER confirm | H advise | I dossier | SPACE day | K save | L load")
        : TEXT("WASD camera  CLICK select  ENTER move/fight  3 field  X convoy  H advise  Z route  SPACE day  K save  L load  V 3D"),
        FLinearColor(0.72f, 0.78f, 0.87f), 147.f, Scale * 0.92f);

    if (!Rules->IsBattleReplayActive() && !Rules->IsLiveBattleActive())
    {
        // Non-interactive threat forecast. No action or resource mutation.
        const float ThreatWidth = FMath::Min(Width - 28.f, 925.f);
        if (ThreatWidth > 160.f)
        {
            DrawRect(FLinearColor(0.19f, 0.075f, 0.04f, 0.93f),
                14.f, 179.f, ThreatWidth, 25.f);
            DrawFitted(Rules->GetRivalForecast(), FLinearColor(1.f, 0.79f, 0.51f),
                22.f, 184.f, ThreatWidth - 18.f, Scale * 0.91f);
        }
    }

    if (Rules->IsBattleReplayActive())
    {
        // TextRender in a real 3D scene can lose contrast against bright
        // geometry. The deterministic HUD mirrors the live stack state.
        const float ColW = FMath::Max(100.f, (Width - 42.f) * 0.5f);
        const float LeftX = 14.f;
        const float RightX = Width * 0.5f + 7.f;
        DrawRect(FLinearColor(0.015f, 0.12f, 0.21f, 0.94f),
            LeftX, 184.f, ColW, 34.f);
        DrawRect(FLinearColor(0.25f, 0.075f, 0.035f, 0.94f),
            RightX, 184.f, ColW, 34.f);
        const int32 LabelLimit = FMath::Max(18, FMath::FloorToInt(ColW / 7.f));
        DrawText(Rules->GetBattleReplayArmyStatus(true).Left(LabelLimit),
            FLinearColor(0.43f, 0.95f, 1.f), LeftX + 8.f, 193.f, Font, Scale);
        DrawText(Rules->GetBattleReplayArmyStatus(false).Left(LabelLimit),
            FLinearColor(1.f, 0.72f, 0.39f), RightX + 8.f, 193.f, Font, Scale);
    }

    const float PanelY = Height - BottomHeight;
    // Mission status is reconstructed from saved campaign state every frame.
    // It never authorizes a movement, recruitment or AI command.
    if (!Rules->IsBattleReplayActive())
    {
        float GoalX, GoalY, GoalW, GoalH;
        GetActionRect(202, GoalX, GoalY, GoalW, GoalH);
        if (GoalW > 0.f)
        {
            const FAIHeroesCampaignProgress Progress = Rules->GetCampaignProgress();
            DrawRect(FLinearColor(0.015f, 0.065f, 0.11f, 0.97f),
                0.f, GoalY - 2.f, Width, GoalH + 4.f);
            float GoalTextRight = GoalX;
            for (const int32 Peer : {203, 204, 205})
            {
                const auto PeerRect = FAIHeroesHUDLayout::Rect(Peer, CachedCanvasSize);
                if (PeerRect.W > 0.f) GoalTextRight = FMath::Min(GoalTextRight, PeerRect.X);
            }
            DrawFitted(Progress.Headline(),
                Progress.bVictory ? FLinearColor(0.8f, 1.f, 0.55f) :
                    FLinearColor(0.44f, 0.93f, 0.9f),
                14.f, GoalY + 6.f, GoalTextRight - 28.f, Scale * 0.92f);
            DrawRect(FLinearColor(0.07f, 0.30f, 0.31f, 0.99f),
                GoalX, GoalY, GoalW, GoalH);
            DrawText(TEXT("GOALS O"), FLinearColor::White,
                GoalX + 8.f, GoalY + 7.f, Font, Scale * 0.9f);
        }
        float ConvoyX, ConvoyY, ConvoyW, ConvoyH;
        GetActionRect(205, ConvoyX, ConvoyY, ConvoyW, ConvoyH);
        if (ConvoyW > 0.f)
        {
            const bool bConfirm = Rules->IsSupplyConvoyPending();
            DrawRect(bConfirm ? FLinearColor(0.12f, 0.38f, 0.22f, 0.99f)
                    : FLinearColor(0.15f, 0.20f, 0.31f, 0.99f),
                ConvoyX, ConvoyY, ConvoyW, ConvoyH);
            DrawText(bConfirm ? TEXT("CONFIRM X") : TEXT("CONVOY X"),
                FLinearColor(0.85f, 0.96f, 1.f),
                ConvoyX + 7.f, ConvoyY + 7.f, Font, Scale * 0.9f);
        }
        float FieldX, FieldY, FieldW, FieldH;
        GetActionRect(204, FieldX, FieldY, FieldW, FieldH);
        if (FieldW > 0.f)
        {
            DrawRect(FLinearColor(0.20f, 0.25f, 0.11f, 0.99f),
                FieldX, FieldY, FieldW, FieldH);
            DrawText(TEXT("FIELD 3"), FLinearColor(0.92f, 1.f, 0.65f),
                FieldX + 8.f, FieldY + 7.f, Font, Scale * 0.9f);
        }
        // The protocol comparison is a peer of GOALS, not another
        // strategic command; mobile touch and mouse use this same rect.
        float LabX, LabY, LabW, LabH;
        GetActionRect(203, LabX, LabY, LabW, LabH);
        if (LabW > 0.f)
        {
            DrawRect(FLinearColor(0.12f, 0.23f, 0.34f, 0.99f),
                LabX, LabY, LabW, LabH);
            DrawText(TEXT("LAB Y"), FLinearColor(0.75f, 0.93f, 1.f),
                LabX + 8.f, LabY + 7.f, Font, Scale * 0.9f);
        }
    }
    DrawRect(FLinearColor(0.015f, 0.023f, 0.046f, 0.94f),
        0.f, PanelY, Width, BottomHeight);
    DrawFitted(TEXT("EVENT: ") + Rules->LastEvent,
        FLinearColor::White, 18.f, PanelY + 11.f, Width - 36.f, Scale);
    DrawFitted(TEXT("HERO ADVICE: ") + Rules->LastHint,
        FLinearColor(0.45f, 0.96f, 0.84f), 18.f, PanelY + 39.f, Width - 36.f, Scale);
    const FString BattleDisplay = Rules->IsBattleReplayActive()
        ? Rules->GetBattleReplayStatus() : Rules->GetReplayDisplayText();
    DrawFitted(BattleDisplay,
        FLinearColor(0.87f, 0.77f, 0.5f), 18.f, PanelY + 67.f, Width - 36.f, Scale);
    DrawText(Width < 500.f ? TEXT("RULE AI") : Width < 700.f ? TEXT("RULE AI | NO LLM") :
        TEXT("LOCAL RULE AI  |  1B / 3B / 8B are themed tiers; no actual LLM loaded"),
        FLinearColor(0.78f, 0.75f, 0.79f), 18.f, PanelY + 89.f, Font, Scale * 0.85f);

    for (int32 Index = 0; Index < 8; ++Index)
    {
        // Grid slot 7 uses distinct action ID 12; utility IDs 7..11
        // stay byte-for-byte compatible with existing UI automation.
        const int32 ActionId = Index == 7 ? 12 : Index;
        float X, Y, W, H;
        GetActionRect(ActionId, X, Y, W, H);
        const bool bActiveRoute = Index == 7 &&
            Rules->GetRoutePreview().IsValid() && !Rules->IsBattleReplayActive();
        DrawRect(Index == 0 ? FLinearColor(0.05f, 0.34f, 0.39f, 0.97f) :
            bActiveRoute ? FLinearColor(0.07f, 0.30f, 0.32f, 0.98f) :
            FLinearColor(0.08f, 0.15f, 0.24f, 0.98f), X, Y, W, H);
        DrawText(Index == 0 && Rules->IsBattleReplayActive()
            ? TEXT("SKIP 3D REPLAY") :
            bActiveRoute ? TEXT("NEXT PLAN Z") : ActionLabels[Index],
            FLinearColor::White, X + 10.f, Y + 11.f, Font, Scale);
    }

    {
        float X, Y, W, H;
        GetActionRect(200, X, Y, W, H);
        DrawRect(FLinearColor(0.08f, 0.24f, 0.28f, 0.98f), X, Y, W, H);
        DrawText(TEXT("RECR N"), FLinearColor(0.65f, 1.f, 0.86f),
            X + 6.f, Y + 11.f, Font, Scale * 0.9f);
    }

    {
        float X, Y, W, H;
        GetActionRect(201, X, Y, W, H);
        DrawRect(FLinearColor(0.20f, 0.24f, 0.12f, 0.98f), X, Y, W, H);
        DrawText(TEXT("HQ + U"), FLinearColor(1.f, 0.91f, 0.55f),
            X + 6.f, Y + 11.f, Font, Scale * 0.9f);
    }

    for (int32 Index = 7; Index < 12; ++Index)
    {
        float X, Y, W, H;
        GetActionRect(Index, X, Y, W, H);
        DrawRect(FLinearColor(0.07f, 0.23f, 0.3f, 0.98f), X, Y, W, H);
        const FString UtilityText = Index == 9
            ? FString::Printf(TEXT("SLOT %d C"), Rules->ActiveSaveSlot)
            : FString(UtilityLabels[Index - 7]);
        DrawText(UtilityText, FLinearColor(0.84f, 0.95f, 1.f),
            X + 6.f, Y + 7.f, Font, Scale * 0.87f);
    }

    if (!Rules->IsBattleReplayActive())
    {
        // Shared helper keeps rendering and hit tests identical. This lives in
        // the bottom information strip, not over the top SAVE/REPLAY utilities.
        const FAIHeroesHUDActionRect R = FAIHeroesAdvisorDossierRules::OpenButton(CachedCanvasSize);
        if (R.W > 0.f)
        {
            DrawRect(FLinearColor(0.06f, 0.25f, 0.31f, 1.f), R.X, R.Y, R.W, R.H);
            DrawText(TEXT("DOSSIER I"), FLinearColor(0.6f, 0.95f, 1.f),
                R.X + 10.f, R.Y + 6.f, Font, Scale * 0.95f);
        }
    }
    if (!Rules->IsBattleReplayActive())
    {
        const auto R = FAIHeroesBasePanelRules::OpenButton(CachedCanvasSize);
        if (R.W > 0)
        {
            DrawRect(FLinearColor(0.17f, 0.25f, 0.12f), R.X, R.Y, R.W, R.H);
            DrawText(TEXT("BASE G"), FLinearColor(1.f, 0.93f, 0.62f),
                R.X + 10, R.Y + 6, Font, Scale * 0.95f);
        }
    }
    if (Rules->IsAdvisorDossierVisible())
    {
        DrawAdvisorDossier(*Rules); // Modal is always the final rendered layer.
    }
}
