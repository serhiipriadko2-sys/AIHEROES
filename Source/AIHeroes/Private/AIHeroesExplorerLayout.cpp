#include "AIHeroesExplorerLayout.h"

FAIHeroesExplorerFrame FAIHeroesExplorerLayout::Frame(const FVector2D& V)
{
    FAIHeroesExplorerFrame F;
    if (!FMath::IsFinite(V.X) || !FMath::IsFinite(V.Y) || V.X < 320 || V.Y < 480 || V.X > 16384 || V.Y > 16384) return F;
    const float W=V.X,H=V.Y;
    F.bSupported=true; F.bWide=W>=1000;
    const bool Tiny=W<640;
    F.Header={0,0,W,F.bWide?66.f:Tiny?112.f:88.f};
    F.Dock={0,H-(Tiny?146.f:92.f),W,Tiny?146.f:92.f};
    F.Inspector=F.bWide ? FAIHeroesHUDActionRect{W-310,84,290,300} :
        FAIHeroesHUDActionRect{12,F.Dock.Y-130,W-24,118};
    F.Map={12,F.Header.H+12,F.bWide?W-338:W-24,
        (F.bWide?F.Dock.Y-12:F.Inspector.Y-12)-(F.Header.H+12)};
    F.Mission={20,F.Header.H+12,FMath::Min(320.f,F.Map.W-16),48};
    F.Menu={FMath::Max(12.f,W-310),F.Header.H+8,FMath::Min(290.f,W-24),FMath::Min(492.f,F.Dock.Y-F.Header.H-16)};
    return F;
}
TArray<int32> FAIHeroesExplorerLayout::Actions(bool MenuOpen)
{
    TArray<int32> Out={700,400,203,1,2};
    if(MenuOpen) Out.Append({202,210,12,200,204,205,7,8,9,10,11});
    else Out.Add(0);
    return Out;
}
FAIHeroesHUDActionRect FAIHeroesExplorerLayout::Button(int32 A,const FVector2D& V,bool Open)
{
    const auto F=Frame(V);if(!F.bSupported)return {};
    const float W=V.X;
    if(A==700)return {W-92,12,76,40};
    const int32 DockIds[]={400,203,1,2};
    for(int32 I=0;I<4;++I)if(A==DockIds[I])
    {
        const int32 Cols=W<640?2:4;
        const float BW=(W-32-8*(Cols-1))/Cols;
        return {16+(I%Cols)*(BW+8),F.Dock.Y+12+(I/Cols)*54,BW,46};
    }
    if(A==0 && !Open)return {F.Inspector.X+14,F.Inspector.Y+F.Inspector.H-60,F.Inspector.W-28,46};
    if(Open)
    {
        const int32 Ids[]={202,210,12,200,204,205,7,8,9,10,11};
        const bool TwoCols=F.Menu.H<480;
        const int32 Cols=TwoCols?2:1;
        const float BW=(F.Menu.W-24-(Cols-1)*8)/Cols;
        const float BH=FMath::Min(38.f,(F.Menu.H-40)/(TwoCols?6:11)-4);
        for(int32 I=0;I<11;++I)if(A==Ids[I])return {F.Menu.X+12+(I%Cols)*(BW+8),F.Menu.Y+32+(I/Cols)*(BH+4),BW,BH};
    }
    return {};
}
int32 FAIHeroesExplorerLayout::Hit(const FVector2D& P,const FVector2D& V,bool Open)
{
    const auto F=Frame(V);if(!F.bSupported || !FMath::IsFinite(P.X) || !FMath::IsFinite(P.Y))return INDEX_NONE;
    for(int32 A:Actions(Open))if(Button(A,V,Open).Contains(P))return A;
    if(F.Header.Contains(P)||F.Dock.Contains(P)||F.Inspector.Contains(P)||F.Mission.Contains(P)||
        (Open&&F.Menu.Contains(P)))return ConsumeAction;
    // An open utility menu consumes outside taps to avoid accidental map orders.
    return Open?ConsumeAction:INDEX_NONE;
}
