#include "AIHeroesHUD.h"

#include "AIHeroesBattleArt.h"
#include "AIHeroesBattleAftermath.h"
#include "AIHeroesBattleFallbackView.h"
#include "AIHeroesGameMode.h"
#include "AIHeroesLiveBattleStage.h"
#include "AIHeroesHUDTextLayout.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"

void AAIHeroesHUD::DrawAmberBattle(const AAIHeroesGameMode& G)
{
    if(!Canvas||!GEngine) return;
    const FVector2D V(Canvas->SizeX,Canvas->SizeY);
    const FLinearColor Ink(.012f,.025f,.028f,1),Card(.025f,.047f,.051f,1);
    const FLinearColor Paper(.94f,.93f,.85f,1),Muted(.66f,.73f,.71f,1);
    const FLinearColor Teal(.16f,.70f,.68f,1),Amber(1.f,.67f,.28f,1),Rust(.92f,.40f,.23f,1);
    UFont* Font=GEngine->GetSmallFont();
    auto Text=[&](const FString& S,float X,float Y,float W,const FLinearColor& Color,float Scale=1.1f)
    {
        auto Measure=[&](const FString& Line){float A=0,B=0;GetTextSize(Line,A,B,Font,Scale);return A;};
        const FString Fit=FAIHeroesHUDTextLayout::FitLine(S,W,Measure);
        if(!Fit.IsEmpty())DrawText(Fit,Color,X,Y,Font,Scale);
    };
    const auto Phase=G.GetLiveBattlePhase();
    const bool Result=Phase==EAIHeroesLiveBattlePhase::Result;
    const bool Paused=Phase==EAIHeroesLiveBattlePhase::Paused;
    const AAIHeroesLiveBattleStage* Stage=G.GetLiveBattlePresentation();
    FAIHeroesGridAction LastAction;
    const bool HasTextAction=!Stage&&!Result&&G.TryGetLatestLiveBattleAction(LastAction);
    const auto Fallback=FAIHeroesBattleFallbackView::Build(
        Stage!=nullptr,Result,Paused,G.GetLiveBattleActionCount(),
        Stage?FString():G.GetLiveArmyStatus(true),
        Stage?FString():G.GetLiveArmyStatus(false),HasTextAction?&LastAction:nullptr);
    const FString Title=Result
        ? G.LastBattleRecord.bVictory ? TEXT("ПОБЕДА") : G.LastBattleRecord.bDefeat ? TEXT("ПОРАЖЕНИЕ") : TEXT("БОЙ ЗАВЕРШЁН")
        : Paused ? TEXT("СРАЖЕНИЕ / ПАУЗА") : TEXT("СРАЖЕНИЕ");
    DrawRect(Ink,0,0,V.X,62);DrawRect(Amber,0,60,V.X,2);
    Text(Title,18,12,V.X-230,Paper,1.55f);
    Text(Stage?TEXT("Вы наблюдаете. Герой командует отрядами.")
        :TEXT("3D-сцена недоступна. Текстовое отображение."),18,38,V.X-36,Muted,.95f);
    const FString Progress=Stage
        ? FString::Printf(TEXT("Раунд %d  /  %dx"),Stage->GetPresentedRound(),
            FMath::RoundToInt(G.GetLiveBattleSpeed()))
        : Fallback.ActionLabel;
    Text(Progress,V.X-195,17,180,Amber,1.1f);
    const float Half=(V.X-36.f)*.5f;
    Text(TEXT("ВАШИ ОТРЯДЫ  /  A"),12,70,Half,Teal,.95f);
    Text(TEXT("ПРОТИВНИК  /  B"),24+Half,70,Half,Rust,.95f);
    if(Stage)
    for(bool Player:{true,false})
    {
        const auto& Army=Stage->GetPresentedArmy(Player);
        for(int32 I=0;I<Army.Num()&&I<2;++I)
        {
            const auto R=FAIHeroesBattleArt::RosterRect(V,Player,I);
            if(R.W<=0)continue;
            const auto& Unit=Army[I];
            const FLinearColor Side=Player?Teal:Rust;
            DrawRect(Card,R.X,R.Y,R.W,R.H);DrawRect(Side,R.X,R.Y,3,R.H);
            const float PortraitSize=R.W>=220?58.f:34.f;
            UTexture2D* Portrait=BattlePortraits.FindRef(Unit.UnitId);
            if(!Portrait)
            {
                const FString Path=FAIHeroesBattleArt::PortraitPath(Unit.UnitId);
                if(!Path.IsEmpty())Portrait=LoadObject<UTexture2D>(nullptr,*Path);
                if(Portrait)BattlePortraits.Add(Unit.UnitId,Portrait);
            }
            if(Portrait)DrawTextureSimple(Portrait,R.X+5,R.Y+3,PortraitSize/256.f,false);
            const float X=R.X+PortraitSize+9,Span=R.W-PortraitSize-17;
            Text(FAIHeroesBattleArt::GroupCode(Player,I)+TEXT("  ")+FAIHeroesBattleArt::UnitName(Unit.UnitId),
                X,R.Y+5,Span,Unit.IsAlive()?Paper:Muted,1.08f);
            Text(FString::Printf(TEXT("%d бойцов   /   %d ОЗ"),Unit.Count(),Unit.TotalHealth),
                X,R.Y+25,Span,Unit.IsAlive()?Side:Muted,1.0f);
            const int32 Initial=Stage->GetInitialHealth(Player,I);
            const float Fraction=Initial>0?FMath::Clamp(float(Unit.TotalHealth)/Initial,0.f,1.f):0.f;
            DrawRect(Ink,X,R.Y+47,Span,5);if(Fraction>0)DrawRect(Side,X,R.Y+47,Span*Fraction,5);
            Text(FString::Printf(TEXT("%s / инициатива %d"),*FAIHeroesBattleArt::Role(Unit.UnitId),Unit.Initiative),
                X,R.Y+55,Span,Muted,.70f);
        }
    }
    if(Fallback.bVisible)
    for(bool Player:{true,false})
    {
        const auto& Lines=Player?Fallback.PlayerRoster:Fallback.EnemyRoster;
        for(int32 I=0;I<Lines.Num()&&I<2;++I)
        {
            const auto R=FAIHeroesBattleArt::RosterRect(V,Player,I);
            if(R.W<=0||R.H<=0)continue;
            const FLinearColor Side=Player?Teal:Rust;
            DrawRect(Card,R.X,R.Y,R.W,R.H);
            DrawRect(Side,R.X,R.Y,3,R.H);
            FString Label,Stats;
            if(Lines[I].Split(TEXT(":"),&Label,&Stats))
            {
                Text(Label,R.X+9,R.Y+11,R.W-18,Paper,.95f);
                Text(Stats.TrimStart(),R.X+9,R.Y+34,R.W-18,Side,.82f);
            }
            else Text(Lines[I],R.X+9,R.Y+16,R.W-18,Muted,.92f);
        }
    }
    const float Bottom=V.Y-126.f;
    DrawRect(Ink,0,Bottom,V.X,126);DrawRect(Amber,0,Bottom,V.X,2);
    const FString Caption=Stage?Stage->GetPresentedCaption():Fallback.Status;
    Text(Caption,16,Bottom+12,V.X-32,Paper,1.15f);
    // Live combat never invents a result or rewards just because its visual
    // stage is missing. Only a committed result can show player casualties.
    const FString Decision=Stage?Stage->GetPresentedDecisionReason():Fallback.DecisionReason;
    const FString Guidance = Result
        ? FAIHeroesBattleAftermath::Build(G.LastBattleRecord).PlayerLine
        : !Decision.IsEmpty() ? Decision
        : Fallback.bVisible
        ? TEXT("Пауза, скорость и досмотреть быстро — кнопки внизу.")
        : V.X>=900?TEXT("Q / E — поворот   R / F — масштаб   Пробел — пауза. A и B — стороны, цифра — отряд.")
        : TEXT("A — ваши, B — враг. Поворот: Q / E. Масштаб: R / F.");
    Text(Guidance,16,Bottom+37,V.X-32,Muted,.9f);
    const FString Labels[]={Paused?TEXT("Продолжить"):TEXT("Пауза"),
        FString::Printf(TEXT("Скорость %dx"),FMath::RoundToInt(G.GetLiveBattleSpeed())),
        V.X<800?TEXT("Завершить"):TEXT("Досмотреть быстро"),Result?TEXT("На карту"):TEXT("Бой идёт")};
    for(int32 I=0;I<4;++I)
    {
        const auto R=FAIHeroesHUDLayout::Rect(100+I,V,true);
        const bool Enabled=I==3?Result:!Result;
        DrawRect(Enabled?(I==3?Amber:Card):FLinearColor(.03f,.036f,.038f),R.X,R.Y,R.W,R.H);
        Text(Labels[I],R.X+9,R.Y+13,R.W-18,Enabled?(I==3?Ink:Paper):Muted,V.X>=800?1.1f:.96f);
    }
}
