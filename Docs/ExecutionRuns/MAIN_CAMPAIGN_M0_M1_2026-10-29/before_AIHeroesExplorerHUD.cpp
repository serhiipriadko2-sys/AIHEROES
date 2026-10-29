#include "AIHeroesHUD.h"
#include "AIHeroesExplorerLayout.h"
#include "AIHeroesExplorerCopy.h"
#include "AIHeroesHUDTextLayout.h"
#include "AIHeroesCampaignGuide.h"
#include "AIHeroesTravelRules.h"
#include "AIHeroesMapLabelLayout.h"
#include "AIHeroesReferenceUI.h"
#include "AIHeroesBattleArt.h"
#include "AIHeroesFogRules.h"
#include "AIHeroesGameMode.h"
#include "AIHeroesWorldNode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "HAL/PlatformTime.h"
#include "GameFramework/PlayerController.h"

namespace AmberHUD
{
const FLinearColor Ink(.0035f,.007f,.011f,1.f),Card(.008f,.014f,.020f,1.f);
const FLinearColor Paper(.94f,.91f,.81f,1.f),Muted(.56f,.61f,.60f,1.f);
const FLinearColor Gold(.83f,.56f,.23f,1.f),Edge(.23f,.15f,.065f,1.f);
const FLinearColor Teal(.22f,.68f,.78f,1.f),Blue(.016f,.063f,.14f,1.f);
FString Site(EAIHeroesNodeType T)
{
    switch(T){
        case EAIHeroesNodeType::Headquarters:return TEXT("Цитадель Искры");
        case EAIHeroesNodeType::ForestMine:return TEXT("Лесопилка");
        case EAIHeroesNodeType::OreMine:return TEXT("Рудник");
        case EAIHeroesNodeType::Outpost:return TEXT("Сторожевая башня");
        case EAIHeroesNodeType::Ruins:return TEXT("Древний архив");
        default:return TEXT("Неизвестный объект");}
}
EAIHeroesGlyph SiteIcon(EAIHeroesNodeType T)
{
    switch(T){case EAIHeroesNodeType::ForestMine:return EAIHeroesGlyph::Wood;
        case EAIHeroesNodeType::OreMine:return EAIHeroesGlyph::Ore;
        case EAIHeroesNodeType::Ruins:return EAIHeroesGlyph::Data;
        default:return EAIHeroesGlyph::Castle;}
}
FString Purpose(EAIHeroesNodeType T)
{
    switch(T){
        case EAIHeroesNodeType::Headquarters:return TEXT("Строительство, найм и развитие героя");
        case EAIHeroesNodeType::ForestMine:return TEXT("После захвата: +15 дерева в день");
        case EAIHeroesNodeType::OreMine:return TEXT("После захвата: +12 руды в день");
        case EAIHeroesNodeType::Outpost:return TEXT("После захвата: +30 золота в день");
        default:return TEXT("Здесь хранятся знания древних машин");}
}
FString MenuName(int32 A)
{
    switch(A){case 202:return TEXT("Задачи  [O]");case 210:return TEXT("Отчёт героя  [I]");
        case 12:return TEXT("План пути  [Z]");case 200:return TEXT("Совет по армии  [N]");
        case 204:return TEXT("Полевой лагерь  [3]");case 205:return TEXT("Обоз  [X]");
        case 7:return TEXT("Сохранить  [K]");case 8:return TEXT("Загрузить  [L]");
        case 9:return TEXT("Сменить слот  [C]");case 10:return TEXT("Журнал боя  [J]");
        case 11:return TEXT("Повтор боя  [V]");default:return TEXT("");}
}
}

bool AAIHeroesHUD::CloseExplorerMenu()
{
    const bool Old=bExplorerMenuOpen;bExplorerMenuOpen=false;return Old;
}
void AAIHeroesHUD::DrawExplorerHUD(const AAIHeroesGameMode& G)
{
    using namespace AmberHUD;
    const auto V=CachedCanvasSize;const auto F=FAIHeroesExplorerLayout::Frame(V);
    if(!F.bSupported)return;
    UFont* Font=GEngine->GetSmallFont();
    auto Text=[this,Font](const FString& S,float X,float Y,float Span,FLinearColor C,float Scale=1.15f)
    {
        const auto Fit=FAIHeroesHUDTextLayout::FitLine(S,Span,[this,Font,Scale](const FString& T)
        {float W=0,H=0;GetTextSize(T,W,H,Font,Scale);return W;});
        if(!Fit.IsEmpty())DrawText(Fit,C,X,Y,Font,Scale);
    };
    auto Panel=[this](const FAIHeroesHUDActionRect& R,const FLinearColor& C)
    {
        if(R.W<=0||R.H<=0)return;
        DrawRect(C,R.X,R.Y,R.W,R.H);
        DrawLine(R.X+5,R.Y,R.X+R.W-5,R.Y,Edge,1.f);
        DrawLine(R.X+5,R.Y+R.H,R.X+R.W-5,R.Y+R.H,Edge,1.f);
        DrawLine(R.X,R.Y+5,R.X,R.Y+R.H-5,Edge,1.f);
        DrawLine(R.X+R.W,R.Y+5,R.X+R.W,R.Y+R.H-5,Edge,1.f);
        for(int32 CX:{0,1})for(int32 CY:{0,1})
        {
            const float X=R.X+CX*R.W,Y=R.Y+CY*R.H,SX=CX?-1.f:1.f,SY=CY?-1.f:1.f;
            DrawLine(X+SX*3,Y+SY*10,X+SX*3,Y+SY*3,Gold,1.f);
            DrawLine(X+SX*3,Y+SY*3,X+SX*10,Y+SY*3,Gold,1.f);
        }
    };
    auto Glyph=[this](EAIHeroesGlyph K,float X,float Y,float Size,const FLinearColor& C)
    {
        for(const auto& L:FAIHeroesReferenceUI::Glyph(K))
            DrawLine(X+L.A.X*Size,Y+L.A.Y*Size,X+L.B.X*Size,Y+L.B.Y*Size,C,1.5f);
    };
    float MX=-1,MY=-1;if(PlayerOwner)PlayerOwner->GetMousePosition(MX,MY);
    auto Button=[&](int32 A,const FString& S,bool Primary=false,bool Enabled=true)
    {
        const auto R=FAIHeroesExplorerLayout::Button(A,V,bExplorerMenuOpen);if(R.W<=0)return;
        const bool Hover=R.Contains({MX,MY});
        Panel(R,!Enabled?FLinearColor(.013f,.019f,.022f):Primary?FLinearColor(.43f,.23f,.055f):
            A==2?Blue:Hover?FLinearColor(.035f,.07f,.10f):Card);
        if(Hover&&Enabled)DrawRect(Gold,R.X+3,R.Y+4,2,R.H-8);
        EAIHeroesGlyph Icon=EAIHeroesGlyph::Scroll;
        const bool HasIcon=A==400||A==203||A==1||A==2;
        if(A==400)Icon=EAIHeroesGlyph::Castle;
        if(A==203)Icon=EAIHeroesGlyph::Hero;
        if(A==2)Icon=EAIHeroesGlyph::Hourglass;
        const bool IconFits=HasIcon&&R.W>=125;
        if(IconFits)Glyph(Icon,R.X+10,R.Y+(R.H-28)*.5f,28,Enabled?Gold:Muted);
        Text(FAIHeroesExplorerCopy::DockLabel(A,R.W,S),R.X+(IconFits?46:12),R.Y+(R.H-16)*.5f,R.W-(IconFits?58:24),Enabled?Paper:Muted,1.08f);
    };

    Panel(F.Header,Ink);
    Glyph(EAIHeroesGlyph::Compass,16,10,38,Gold);
    Text(TEXT("AIHEROES"),63,9,F.bWide?220:V.X-180,Gold,F.bWide?1.9f:1.55f);
    Text(TEXT("Рубежи Искры"),64,36,F.bWide?220:V.X-190,Paper,.95f);
    Button(700,bExplorerMenuOpen?TEXT("Закрыть"):TEXT("Меню"));
    const float DayX=F.bWide?285.f:V.X-224.f;
    if(V.X>=640)
    {
        Glyph(EAIHeroesGlyph::Sun,DayX,14,26,Gold);
        Text(FString::Printf(TEXT("День %d"),G.Day),DayX+34,11,92,Paper,1.15f);
        Text(FString::Printf(TEXT("Неделя %d"),(G.Day-1)/7+1),DayX+34,34,92,Muted,.85f);
    }
    const int32 Values[]={G.Gold,G.Wood,G.Ore,G.DataFragments};
    const TCHAR* Names[]={TEXT("Золото"),TEXT("Дерево"),TEXT("Руда"),TEXT("Данные")};
    const EAIHeroesGlyph Icons[]={EAIHeroesGlyph::Gold,EAIHeroesGlyph::Wood,EAIHeroesGlyph::Ore,EAIHeroesGlyph::Data};
    for(int32 I=0;I<4;++I)
    {
        if(F.bWide)
        {
            const float W=(V.X-532.f)/4.f,X=420.f+I*W;
            Glyph(Icons[I],X,19,27,I==3?Teal:Gold);
            Text(Names[I],X+36,10,W-42,Muted,.85f);
            Text(FString::FromInt(Values[I]),X+36,29,W-42,Paper,1.3f);
            if(I==0)Text(FString::Printf(TEXT("+%d / день"),G.GetDailyGoldIncome()),X+36,49,W-42,Teal,.72f);
        }
        else
        {
            const int32 Cols=V.X<640?2:4;
            const float W=(V.X-32.f)/Cols,X=16+(I%Cols)*W,Y=56+(I/Cols)*25;
            Glyph(Icons[I],X,Y,19,I==3?Teal:Gold);
            Text(FString::Printf(TEXT("%s %d"),Names[I],Values[I]),X+26,Y+1,W-30,Paper,.96f);
        }
    }
    if(V.X<640)Text(FString::Printf(TEXT("День %d"),G.Day),V.X-175,36,70,Muted,.85f);

    // One player-facing objective source: the actual campaign victory rules.
    const auto Progress=G.GetCampaignProgress();
    const auto* N=G.SelectedNode.Get();
    const bool Valid=IsValid(N)&&G.Nodes.Contains(G.SelectedNode)&&
        G.IsNodeDiscovered(FName(*N->GetDisplayName()));
    const auto Path=G.GetSelectedTravelPath();
    const bool Ready=Valid&&Path.IsReachableThisTurn()&&!G.IsStrategicTravelMoving();
    FAIHeroesCampaignGuideContext GuideContext;
    GuideContext.SelectedId=Valid?FName(*N->GetDisplayName()):NAME_None;
    GuideContext.bTravelMoving=G.IsStrategicTravelMoving();
    GuideContext.bRouteReady=Ready;
    GuideContext.bNeedsMoreMoves=Valid&&Path.Status==EAIHeroesAdventurePathStatus::NeedsMoreMoves;
    GuideContext.MissingMoves=Path.MissingMoves;
    GuideContext.RouteTotalCost=Valid?Path.TotalCost:0;
    GuideContext.FullDayMoveBudget=4; // Authoritative EndTurn restores 4.
    // A long Road V2 route can require a player-selected owned waypoint.
    // The name is supplied only when the route itself and its intermediate
    // ownership are already visible; no hidden map state is exposed.
    if(GuideContext.bNeedsMoreMoves &&
       Path.TotalCost>GuideContext.FullDayMoveBudget &&
       G.TravelRuleVersion==FAIHeroesTravelRules::RoadVersion)
    {
        for(int32 I=1;I+1<Path.Nodes.Num();++I)
        {
            const FName StopId=Path.Nodes[I];
            for(const AAIHeroesWorldNode* Stop:G.Nodes)
            {
                if(IsValid(Stop)&&Stop->bOwned&&!Stop->IsHidden()&&
                   G.IsNodeDiscovered(StopId)&&
                   FName(*Stop->GetDisplayName())==StopId)
                {
                    GuideContext.IntermediateOwnedSite=StopId;
                    break;
                }
            }
            if(!GuideContext.IntermediateOwnedSite.IsNone())break;
        }
    }
    GuideContext.bHasArmy=false;
    for(const auto& Unit:G.PlayerArmy)if(Unit.IsAlive())GuideContext.bHasArmy=true;
    for(const AAIHeroesWorldNode* Site:G.Nodes)
        if(IsValid(Site)&&!Site->IsHidden()&&
           FName(*Site->GetDisplayName())==Progress.NextFocusId&&
           G.IsNodeDiscovered(Progress.NextFocusId))
            GuideContext.bTargetDiscovered=true;
    const auto Guide=FAIHeroesCampaignGuide::Build(Progress,GuideContext);
    const bool MissionHover=F.Mission.Contains({MX,MY});
    Panel(F.Mission,MissionHover?Blue:Card);
    Glyph(EAIHeroesGlyph::Compass,F.Mission.X+10,F.Mission.Y+12,23,Gold);
    Text(Guide.Heading,F.Mission.X+41,F.Mission.Y+6,F.Mission.W-50,Gold,.85f);
    Text(Guide.NextStep,F.Mission.X+41,F.Mission.Y+25,F.Mission.W-50,Paper,.96f);

    TArray<FAIHeroesHUDActionRect> Occupied={F.Mission};
    if(F.Overview.W>0)Occupied.Add(F.Overview);
    if(PlayerOwner)for(const AAIHeroesWorldNode* MapSite:G.Nodes)
    {
        if(!IsValid(MapSite)||MapSite->IsHidden()||!G.IsNodeDiscovered(FName(*MapSite->GetDisplayName())))continue;
        FVector2D At;
        if(!PlayerOwner->ProjectWorldLocationToScreen(MapSite->GetActorLocation()+FVector(0,0,250),At)||!F.Map.Contains(At))continue;
        FAIHeroesHUDActionRect R{static_cast<float>(At.X)-92,static_cast<float>(At.Y)-18,184,34};
        R.X=FMath::Clamp(R.X,F.Map.X,F.Map.X+F.Map.W-R.W);
        R.Y=FMath::Clamp(R.Y,F.Map.Y,F.Map.Y+F.Map.H-R.H);
        bool Free=true;for(const auto& O:Occupied)Free&=!FAIHeroesMapLabelLayout::Overlaps(R,O,4);
        if(!Free)continue;Occupied.Add(R);
        const bool Selected=G.SelectedNode.Get()==MapSite;
        Panel(R,Selected?Blue:Ink);
        Glyph(SiteIcon(MapSite->NodeType),R.X+7,R.Y+5,24,Selected?Gold:MapSite->bOwned?Teal:Paper);
        Text(Site(MapSite->NodeType),R.X+38,R.Y+9,R.W-45,Selected?Gold:MapSite->bOwned?Teal:Paper,.98f);
    }

    // A schematic overview of the known road graph, NOT a painted full-map reveal.
    if(F.Overview.W>0)
    {
        FAIHeroesAdvisorSnapshot S;S.TravelRuleVersion=G.TravelRuleVersion;
        S.bFogEnabled=true;S.ExploredSitesMask=0;S.HeroPosition=G.GetHeroStrategicLocation();
        for(const AAIHeroesWorldNode* OverviewSite:G.Nodes)
        {
            if(!IsValid(OverviewSite))continue;
            FAIHeroesAdvisorNode A;A.Id=FName(*OverviewSite->GetDisplayName());A.Type=OverviewSite->NodeType;
            A.Location=OverviewSite->GetActorLocation();A.bOwned=OverviewSite->bOwned;
            if(G.IsNodeDiscovered(A.Id)&&!OverviewSite->IsHidden())
            {S.ExploredSitesMask|=FAIHeroesFogRules::SiteBit(A.Id);A.Guards=OverviewSite->Defenders;}
            S.Nodes.Add(A);
        }
        const auto O=FAIHeroesReferenceUI::Overview(S);
        Panel(F.Overview,Ink);
        Text(TEXT("РАЗВЕДАННЫЕ ПУТИ"),F.Overview.X+12,F.Overview.Y+10,F.Overview.W-24,Gold,.84f);
        const float X=F.Overview.X+16,Y=F.Overview.Y+36,W=F.Overview.W-32,H=F.Overview.H-58;
        auto P=[&](FVector2D UV){return FVector2D(X+UV.X*W,Y+UV.Y*H);};
        if(O.bValid)
        {
            for(const auto& L:O.Links){const auto A=P(O.Marks[L.X].UV),B=P(O.Marks[L.Y].UV);DrawLine(A.X,A.Y,B.X,B.Y,Edge,2.f);}
            for(const auto& M:O.Marks){const auto At=P(M.UV);Glyph(SiteIcon(M.Type),At.X-8,At.Y-8,16,M.bOwned?Teal:Gold);}
            const auto Hero=P(O.HeroUV);DrawLine(Hero.X-6,Hero.Y,Hero.X,Hero.Y-6,Paper,2.f);
            DrawLine(Hero.X,Hero.Y-6,Hero.X+6,Hero.Y,Paper,2.f);DrawLine(Hero.X+6,Hero.Y,Hero.X,Hero.Y+6,Paper,2.f);
            DrawLine(Hero.X,Hero.Y+6,Hero.X-6,Hero.Y,Paper,2.f);
            Text(FString::Printf(TEXT("Открыто %d / 5  ·  север ↑"),O.Marks.Num()),F.Overview.X+12,F.Overview.Y+F.Overview.H-21,F.Overview.W-24,Muted,.82f);
        }
        else Text(TEXT("Обзор недоступен"),X,Y,W,Muted,.9f);
    }

    Panel(F.Inspector,Card);

    const float X=F.Inspector.X+16,Y=F.Inspector.Y+14,Span=F.Inspector.W-32;
    FString CTA=TEXT("Выберите объект на карте");
    if(Valid)
    {
        int32 Guards=0;for(const auto& U:N->Defenders)if(U.IsAlive())Guards+=U.Count();
        const bool Hostile=!N->bOwned&&Guards>0;
        if(F.bWide)Glyph(SiteIcon(N->NodeType),X,Y,31,Gold);
        Text(Site(N->NodeType),X+(F.bWide?43:0),Y+2,Span-(F.bWide?43:0),Paper,F.bWide?1.40f:1.3f);
        if(F.bWide)
        {
            Text(N->bOwned?TEXT("ВАША ТЕРРИТОРИЯ"):Hostile?TEXT("ПОД ОХРАНОЙ"):TEXT("СВОБОДНАЯ ТЕРРИТОРИЯ"),X,Y+42,Span,N->bOwned?Teal:Gold,.95f);
            DrawLine(X,Y+69,X+Span,Y+69,Edge,1.f);
            Text(Purpose(N->NodeType),X,Y+82,Span,Paper,1.04f);
            Text(FString::Printf(TEXT("Охрана: %d   ·   Ходы: %d / 4"),Guards,G.MovesLeft),X,Y+113,Span,Paper,1.08f);
            Text(!Hostile?TEXT("Можно посетить без боя"):G.ProtocolTier==0?TEXT("Риск неизвестен: герой ещё учится"):TEXT("Спросите героя перед нападением"),X,Y+145,Span,Muted,1.02f);
            Text(Guide.FocusId==GuideContext.SelectedId?Guide.Detail:TEXT("Вы выбираете путь. ИИ ведёт бой."),X,Y+178,Span,Teal,.92f);
        }
        else Text(FString::Printf(TEXT("%s  /  охрана %d  /  ходы %d из 4"),N->bOwned?TEXT("Ваш объект"):TEXT("Не захвачен"),Guards,G.MovesLeft),X,Y+28,Span,Muted,1.f);
        if(Ready)CTA=FString::Printf(TEXT("%s  /  %d хода"),Hostile?TEXT("Атаковать"):TEXT("Перейти"),Path.TotalCost);
        else if(G.IsStrategicTravelMoving())CTA=TEXT("Герой в пути...");
        else if(Path.Status==EAIHeroesAdventurePathStatus::NeedsMoreMoves)
            CTA=Path.TotalCost>GuideContext.FullDayMoveBudget?
                TEXT("Нужна остановка по пути"):
                FString::Printf(TEXT("Не хватает ходов: %d"),Path.MissingMoves);
        else CTA=TEXT("Путь закрыт охраной");
    }
    else
    {
        Text(TEXT("Начните с карты"),X,Y,Span,Paper,1.4f);
        Text(TEXT("Нажмите на лесопилку или рудник"),X,Y+32,Span,Muted,1.f);
        if(F.bWide){Text(Guide.Detail,X,Y+85,Span,Paper);Text(TEXT("Нажмите «Цели», чтобы увидеть все задачи"),X,Y+120,Span,Muted,.88f);Text(TEXT("Игровой приказ всегда подтверждаете вы"),X,Y+155,Span,Teal,.88f);}
    }
    if(!bExplorerMenuOpen)Button(0,CTA,true,Ready);

    if(F.Hero.W>0)
    {
        Panel(F.Hero,Ink);const float HX=F.Hero.X+12,HY=F.Hero.Y+12,HW=F.Hero.W-24;
        const TCHAR* Tiers[]={TEXT("Семя"),TEXT("Аналитик"),TEXT("Память"),TEXT("Планировщик")};
        Glyph(EAIHeroesGlyph::Hero,HX,HY,28,Gold);
        Text(FString::Printf(TEXT("Искра  /  %s"),Tiers[FMath::Clamp(G.ProtocolTier,0,3)]),HX+38,HY+2,HW-38,Paper,1.12f);
        Text(TEXT("АРМИЯ ГЕРОЯ"),HX,HY+37,HW,Gold,.85f);
        const int32 Show=FMath::Min(G.PlayerArmy.Num(),2);
        for(int32 I=0;I<Show;++I)
        {
            const auto& U=G.PlayerArmy[I];const float RY=HY+58+I*59;
            UTexture2D* Portrait=BattlePortraits.FindRef(U.UnitId);
            if(!Portrait){const FString PP=FAIHeroesBattleArt::PortraitPath(U.UnitId);if(!PP.IsEmpty())Portrait=LoadObject<UTexture2D>(nullptr,*PP);if(Portrait)BattlePortraits.Add(U.UnitId,Portrait);}
            Panel({HX,RY,48,48},Card);
            if(Portrait)DrawTextureSimple(Portrait,HX+2,RY+2,44.f/256.f,false);
            else Glyph(EAIHeroesGlyph::Army,HX+8,RY+8,32,Gold);
            Text(FString::Printf(TEXT("%s · %d"),*FAIHeroesBattleArt::UnitName(U.UnitId),U.Count()),HX+58,RY+3,HW-58,Paper,1.05f);
            Text(FString::Printf(TEXT("ОЗ %d  ·  атака %d  ·  защита %d"),U.TotalHealth,U.Attack,U.Defense),HX+58,RY+26,HW-58,Muted,.85f);
        }
        if(Show==0)Text(TEXT("Армия пуста. Найм доступен на базе."),HX,HY+60,HW,Muted,.95f);
        Text(G.PlayerArmy.Num()>2?FString::Printf(TEXT("Ещё отрядов: %d"),G.PlayerArmy.Num()-2):TEXT("ИИ по правилам · модель не подключена"),HX,F.Hero.Y+F.Hero.H-21,HW,Muted,.78f);
    }
    Panel(F.Dock,Ink);
    Button(400,TEXT("Постройки [G]"));Button(203,TEXT("Тактики [Y]"));Button(1,TEXT("Спросить [H]"));Button(2,TEXT("Конец хода"));
    const double Now=FPlatformTime::Seconds();
    if(ExplorerLastEvent!=G.LastEvent){ExplorerLastEvent=G.LastEvent;ExplorerToastUntil=Now+4.5;}
    const FString Footer=Now<ExplorerToastUntil?TEXT("Событие: ")+FAIHeroesExplorerCopy::EventText(ExplorerLastEvent,V.X):
        FString::Printf(TEXT("Ходы %d / 4   |   WASD — камера, Q / E — поворот, R / F — масштаб   |   ИИ по правилам"),G.MovesLeft);
    Text(Footer,16,F.Dock.Y+F.Dock.H-24,V.X-32,Muted,.9f);
    if(bExplorerMenuOpen)
    {
        Panel(F.Menu,Ink);Text(TEXT("ЭКСПЕДИЦИЯ И УПРАВЛЕНИЕ"),F.Menu.X+12,F.Menu.Y+10,F.Menu.W-24,Gold,.9f);
        for(int32 A:{202,210,12,200,204,205,7,8,9,10,11})Button(A,MenuName(A));
    }
    if(G.IsAdvisorDossierVisible())DrawAdvisorDossier(G);
}
