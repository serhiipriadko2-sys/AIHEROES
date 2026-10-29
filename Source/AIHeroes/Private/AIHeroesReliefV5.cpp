#include "AIHeroesReliefV5.h"

namespace
{
struct FReliefPoint{double X,Y;};
struct FReliefHill{double X,Y,Amplitude,Sx,Sy;};
constexpr FReliefPoint Sites[]={
    {-6.5,0.},{-1.,-6.5},{1.5,6.},{7.,-3.},{7.5,6.5}
};
constexpr int32 Links[][2]={{0,1},{0,2},{1,3},{2,4},{3,4}};
constexpr FReliefHill Hills[]={
    {-11,8,1.3,2.8,3.5},{-10,-10,1.55,3.2,3.4},
    {-2.5,12.6,1.35,3.6,2.5},{10.8,11.0,1.6,3.3,3.0},
    {12.2,-8,1.4,2.9,3.5},{3.7,-12.6,1.25,3.5,2.3},
    {-13,1.5,1.0,2.3,4.2},{12.7,0,1.15,2.7,3.5}
};
double Smooth(double A,double B,double V)
{
    const double T=FMath::Clamp((V-A)/(B-A),0.0,1.0);
    return T*T*(3.0-2.0*T);
}
double RoadDistance(double X,double Y,FReliefPoint A,FReliefPoint B)
{
    const double Dx=B.X-A.X,Dy=B.Y-A.Y,Den=Dx*Dx+Dy*Dy;
    const double T=FMath::Clamp(((X-A.X)*Dx+(Y-A.Y)*Dy)/Den,0.0,1.0);
    return FMath::Sqrt(FMath::Square(X-A.X-T*Dx)+FMath::Square(Y-A.Y-T*Dy));
}
double HeightMetres(double X,double Y)
{
    const double Edge=FMath::Max(FMath::Abs(X),FMath::Abs(Y));
    if(Edge>13.0)
        return -0.25-(Edge-13.0)*.24+.027*FMath::Sin(X*2.37+Y*1.44);

    double NearestSite=BIG_NUMBER,NearestRoad=BIG_NUMBER;
    for(const FReliefPoint& Site:Sites)
        NearestSite=FMath::Min(NearestSite,FMath::Sqrt(
            FMath::Square(X-Site.X)+FMath::Square(Y-Site.Y)));
    for(const auto& Link:Links)
        NearestRoad=FMath::Min(NearestRoad,RoadDistance(
            X,Y,Sites[Link[0]],Sites[Link[1]]));
    const double Mask=Smooth(2.35,4.5,NearestSite)*
        (.12+.88*Smooth(1.25,3.8,NearestRoad));
    double Peaks=0.0;
    for(const FReliefHill& Hill:Hills)
        Peaks+=Hill.Amplitude*FMath::Exp(
            -FMath::Square((X-Hill.X)/Hill.Sx)
            -FMath::Square((Y-Hill.Y)/Hill.Sy));
    const double Fine=.07*FMath::Sin(.98*X+1.17*Y)*
        FMath::Cos(1.29*X-.64*Y);
    double Base=-.12+(Peaks+Fine)*Mask;

    const double Angle=FMath::Atan2(5.5,6.5);
    const double C=FMath::Cos(Angle),S=FMath::Sin(Angle);
    const double Lx=(X+3.75)*C+(Y+3.25)*S;
    const double Ly=-(X+3.75)*S+(Y+3.25)*C;
    if(FMath::Abs(Lx)<5.6)
    {
        const double Taper=Smooth(0.,.7,5.6-FMath::Abs(Lx));
        const double Centre=.13*FMath::Sin(Lx*.74)+.08*FMath::Cos(1.47*Lx);
        const double Half=.64+.07*FMath::Sin(.67*Lx+.4);
        const double Dist=FMath::Abs(Ly-Centre);
        double River=Base;
        if(Dist<=Half-.28)River=-.50;
        else if(Dist<=Half)River=-.50+.625*Smooth(Half-.28,Half,Dist);
        else if(Dist<=Half+.27)River=.125+.01*Smooth(Half,Half+.27,Dist);
        else if(Dist<=Half+1.12)
            River=.135+(Base-.135)*Smooth(Half+.27,Half+1.12,Dist);
        Base+=(River-Base)*Taper;
    }
    return Base;
}
}
double FAIHeroesReliefV5::HeightCm(double Xcm,double Ycm)
{
    if(!FMath::IsFinite(Xcm)||!FMath::IsFinite(Ycm)||
       FMath::Abs(Xcm)>1700.||FMath::Abs(Ycm)>1700.)
        return -12.;
    return HeightMetres(Xcm*.01,Ycm*.01)*100.;
}
const TCHAR* FAIHeroesReliefV5::PlatformMeshName()
{
#if PLATFORM_ANDROID
    return TEXT("SM_IslandReliefV5_Mobile");
#else
    return TEXT("SM_IslandReliefV5");
#endif
}