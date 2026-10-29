#include "AIHeroesHUD.h"
#include "AIHeroesExplorerLayout.h"
#include "AIHeroesHUDTextLayout.h"
#include "AIHeroesMapLabelLayout.h"
#include "AIHeroesGameMode.h"
#include "AIHeroesWorldNode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "HAL/PlatformTime.h"
#include "GameFramework/PlayerController.h"

namespace AmberHUD
{
const FLinearColor Ink(0.018f,0.028f,0.031f,0.97f);
const FLinearColor Card(0.035f,0.054f,0.056f,0.98f);
const FLinearColor Paper(0.91f,0.88f,0.76f,1.f);
const FLinearColor Muted(0.53f,0.62f,0.59f,1.f);
const FLinearColor Amber(0.92f,0.57f,0.20f,1.f);
const FLinearColor Teal(0.33f,0.79f,0.72f,1.f);
FString Site(EAIHeroesNodeType T)
{
    switch(T){
        case EAIHeroesNodeType::Headquarters:return TEXT("Цитадель Искры");
        case EAIHeroesNodeType::ForestMine:return TEXT("Лесопилка");
        case EAIHeroesNodeType::OreMine:return TEXT("Рудник");
        case EAIHeroesNodeType::Outpost:return TEXT("Сторожевая башня");
        case EAIHeroesNodeType::Ruins:return TEXT("Древний архив");
        default:return TEXT("Неизвестный объект");
    }
}
FString Purpose(EAIHeroesNodeType T)
{
    switch(T){
        case EAIHeroesNodeType::Headquarters:return TEXT("Строительство, найм и развитие героя");
        case EAIHeroesNodeType::ForestMine:return TEXT("Доход после захвата: +15 дерева в день");
        case EAIHeroesNodeType::OreMine:return TEXT("Доход после захвата: +12 руды в день");
        case EAIHeroesNodeType::Outpost:return TEXT("Доход после захвата: +30 золота в день");
        default:return TEXT("Цель экспедиции: знания древних машин");
    }
}
FString MenuName(int32 A)
{
    switch(A){
        case 202:return TEXT("Задачи  [O]");case 210:return TEXT("Отчёт героя  [I]");
        case 12:return TEXT("План пути  [Z]");case 200:return TEXT("Совет по армии  [N]");
        case 204:return TEXT("Полевой лагерь  [3]");case 205:return TEXT("Обоз  [X]");
        case 7:return TEXT("Сохранить  [K]");case 8:return TEXT("Загрузить  [L]");
        case 9:return TEXT("Сменить слот  [C]");case 10:return TEXT("Журнал боя  [J]");
        case 11:return TEXT("Повтор боя  [V]");default:return TEXT("");
    }
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
    {DrawRect(C,R.X,R.Y,R.W,R.H);};
    float MX=-1,MY=-1;if(PlayerOwner)PlayerOwner->GetMousePosition(MX,MY);
    auto Button=[&](int32 A,const FString& S,bool Primary=false,bool Enabled=true)
    {
        const auto R=FAIHeroesExplorerLayout::Button(A,V,bExplorerMenuOpen);if(R.W<=0)return;
        const bool Hover=R.Contains({MX,MY});
        Panel(R,!Enabled?FLinearColor(.055f,.067f,.063f):Primary?Amber:
            Hover?FLinearColor(.11f,.20f,.19f):FLinearColor(.065f,.105f,.106f));
        if(Hover&&Enabled)DrawRect(Teal,R.X,R.Y,3,R.H);
        Text(S,R.X+12,R.Y+(R.H-16)*.5f,R.W-24,!Enabled?Muted:Primary?Ink:Paper,1.10f);
    };
    Panel(F.Header,Ink);DrawRect(Amber,0,F.Header.H-2,V.X,2);
    Text(TEXT("РУБЕЖИ ИСКРЫ"),20,12,F.bWide?330:V.X-132,Paper,F.bWide?1.65f:1.3f);
    if(F.bWide)Text(TEXT("Зелёная низина / экспедиция 01"),20,40,330,Muted,.95f);
    const FString Resources=FString::Printf(TEXT("День %d    Золото %d    Дерево %d    Руда %d    Данные %d"),G.Day,G.Gold,G.Wood,G.Ore,G.DataFragments);
    if(V.X<640)
    {
        Text(FString::Printf(TEXT("День %d  /  Золото %d  /  Дерево %d"),G.Day,G.Gold,G.Wood),20,58,V.X-40,Paper,1.f);
        Text(FString::Printf(TEXT("Руда %d  /  Данные %d"),G.Ore,G.DataFragments),20,80,V.X-40,Paper,1.f);
    }
    else Text(Resources,F.bWide?350:20,F.bWide?23:58,F.bWide?V.X-458:V.X-40,Paper,F.bWide?1.15f:1.0f);
    Button(700,bExplorerMenuOpen?TEXT("Закрыть"):TEXT("Меню"));
    Panel(F.Mission,Card);
    FString Objective=TEXT("Выберите лесопилку и разведайте путь");
    for(const AAIHeroesWorldNode* N:G.Nodes)if(IsValid(N)&&N->NodeType==EAIHeroesNodeType::ForestMine&&N->bOwned)Objective=TEXT("Следующая цель: захватите рудник");
    for(const AAIHeroesWorldNode* N:G.Nodes)if(IsValid(N)&&N->NodeType==EAIHeroesNodeType::OreMine&&N->bOwned)Objective=TEXT("Откройте путь к древнему архиву");
    if(G.GetCampaignProgress().bVictory)Objective=TEXT("Экспедиция завершена!");
    Text(TEXT("ЦЕЛЬ ЭКСПЕДИЦИИ"),F.Mission.X+12,F.Mission.Y+6,F.Mission.W-24,Amber,.85f);
    Text(Objective,F.Mission.X+12,F.Mission.Y+25,F.Mission.W-24,Paper,1.0f);

    // Simple, sparse site badges. Never label undiscovered sites.
    TArray<FAIHeroesHUDActionRect> Occupied={F.Mission};
    if(PlayerOwner)
    for(const AAIHeroesWorldNode* N:G.Nodes)
    {
        if(!IsValid(N)||N->IsHidden()||!G.IsNodeDiscovered(FName(*N->GetDisplayName())))continue;
        FVector2D At;
        if(!PlayerOwner->ProjectWorldLocationToScreen(N->GetActorLocation()+FVector(0,0,250),At)||!F.Map.Contains(At))continue;
        FAIHeroesHUDActionRect R{static_cast<float>(At.X)-75,static_cast<float>(At.Y)-18,150,32};
        R.X=FMath::Clamp(R.X,F.Map.X,F.Map.X+F.Map.W-R.W);
        R.Y=FMath::Clamp(R.Y,F.Map.Y,F.Map.Y+F.Map.H-R.H);
        bool Free=true;for(const auto& O:Occupied)Free&=!FAIHeroesMapLabelLayout::Overlaps(R,O,4);
        if(!Free)continue;Occupied.Add(R);
        const bool Selected=G.SelectedNode.Get()==N;
        Panel(R,Selected?FLinearColor(.065f,.19f,.18f,.98f):Ink);
        DrawRect(Selected?Amber:N->bOwned?Teal:Muted,R.X,R.Y,3,R.H);
        Text(Site(N->NodeType),R.X+10,R.Y+8,R.W-20,Selected?Paper:N->bOwned?Teal:Paper,1.f);
    }

    Panel(F.Inspector,Card);DrawRect(Amber,F.Inspector.X,F.Inspector.Y,3,F.Inspector.H);
    const auto* N=G.SelectedNode.Get();
    const bool Valid=IsValid(N)&&G.Nodes.Contains(G.SelectedNode)&&G.IsNodeDiscovered(FName(*N->GetDisplayName()));
    const auto Path=G.GetSelectedTravelPath();
    const bool Ready=Valid&&Path.IsReachableThisTurn()&&!G.IsStrategicTravelMoving();
    const float X=F.Inspector.X+16,Y=F.Inspector.Y+14,Span=F.Inspector.W-32;
    FString CTA=TEXT("Выберите объект на карте");
    if(Valid)
    {
        int32 Guards=0;for(const auto& U:N->Defenders)if(U.IsAlive())Guards+=U.Count();
        const bool Hostile=!N->bOwned&&Guards>0;
        Text(Site(N->NodeType),X,Y,Span,Paper,F.bWide?1.65f:1.3f);
        if(F.bWide)
        {
            Text(N->bOwned?TEXT("ВАША ТЕРРИТОРИЯ"):Hostile?TEXT("ПОД ОХРАНОЙ"):TEXT("СВОБОДНАЯ ТЕРРИТОРИЯ"),X,Y+34,Span,N->bOwned?Teal:Amber,.95f);
            Text(Purpose(N->NodeType),X,Y+66,Span,Muted,1.05f);
            Text(FString::Printf(TEXT("Охрана: %d    Движение: %d / 4"),Guards,G.MovesLeft),X,Y+98,Span,Paper,1.1f);
            Text(!Hostile?TEXT("Можно посетить без боя"):G.ProtocolTier==0?TEXT("Исход боя: пока не оценён"):TEXT("Спросите героя перед нападением"),X,Y+130,Span,Muted,1.05f);
            Text(TEXT("Вы решаете. Герой ведёт бой."),X,Y+160,Span,Teal,1.0f);
        }
        else Text(FString::Printf(TEXT("%s  /  охрана %d  /  ходы %d из 4"),N->bOwned?TEXT("Ваш объект"):TEXT("Не захвачен"),Guards,G.MovesLeft),X,Y+28,Span,Muted,1.f);
        if(Ready)CTA=FString::Printf(TEXT("%s  /  %d хода"),Hostile?TEXT("Атаковать"):TEXT("Перейти"),Path.TotalCost);
        else if(G.IsStrategicTravelMoving())CTA=TEXT("Герой в пути...");
        else if(Path.Status==EAIHeroesAdventurePathStatus::NeedsMoreMoves)CTA=FString::Printf(TEXT("Не хватает ходов: %d"),Path.MissingMoves);
        else CTA=TEXT("Путь закрыт охраной");
    }
    else
    {
        Text(TEXT("Начните с карты"),X,Y,Span,Paper,1.4f);
        Text(TEXT("Нажмите на лесопилку или рудник"),X,Y+32,Span,Muted,1.f);
        if(F.bWide){Text(TEXT("1  Выберите цель"),X,Y+85,Span,Paper);Text(TEXT("2  Спросите героя"),X,Y+120,Span,Paper);Text(TEXT("3  Подтвердите поход"),X,Y+155,Span,Paper);}
    }
    if(!bExplorerMenuOpen)Button(0,CTA,true,Ready);
    Panel(F.Dock,Ink);DrawRect(FLinearColor(.18f,.25f,.22f),0,F.Dock.Y,V.X,1);
    Button(400,TEXT("База  [G]"));Button(203,TEXT("Герой  [Y]"));Button(1,TEXT("Спросить  [H]"));Button(2,TEXT("Конец хода"));
    const double Now=FPlatformTime::Seconds();
    if(ExplorerLastEvent!=G.LastEvent){ExplorerLastEvent=G.LastEvent;ExplorerToastUntil=Now+4.5;}
    const FString Footer=Now<ExplorerToastUntil ? TEXT("Событие: ")+ExplorerLastEvent :
        FString::Printf(TEXT("Ходы %d / 4   |   Герой: %s   |   ИИ по правилам, модель не загружена"),G.MovesLeft,G.ProtocolTier==0?TEXT("Искра-семя"):TEXT("Аналитик"));
    Text(Footer,16,F.Dock.Y+F.Dock.H-24,V.X-32,Muted,.9f);
    if(bExplorerMenuOpen)
    {
        Panel(F.Menu,Ink);Text(TEXT("ЭКСПЕДИЦИЯ И УПРАВЛЕНИЕ"),F.Menu.X+12,F.Menu.Y+10,F.Menu.W-24,Amber,.9f);
        for(int32 A:{202,210,12,200,204,205,7,8,9,10,11})Button(A,MenuName(A));
    }
    if(G.IsAdvisorDossierVisible())DrawAdvisorDossier(G);
}
