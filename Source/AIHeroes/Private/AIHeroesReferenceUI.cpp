#include "AIHeroesReferenceUI.h"
#include "AIHeroesFogRules.h"
#include "AIHeroesTravelRules.h"

TArray<FAIHeroesGlyphLine> FAIHeroesReferenceUI::Glyph(EAIHeroesGlyph K)
{
    TArray<FAIHeroesGlyphLine> Out;
    auto L=[&](float X,float Y,float U,float V){Out.Add({{X,Y},{U,V}});};
    auto Ring=[&](float X,float Y,float RX,float RY)
    {
        for(int I=0;I<16;++I)
        {
            const double A=I*2.0*PI/16.0,B=(I+1)*2.0*PI/16.0;
            L(X+RX*FMath::Cos(A),Y+RY*FMath::Sin(A),X+RX*FMath::Cos(B),Y+RY*FMath::Sin(B));
        }
    };
    switch(K)
    {
        case EAIHeroesGlyph::Gold:
            Ring(.5,.47,.34,.26);L(.16,.47,.16,.65);L(.84,.47,.84,.65);
            L(.16,.65,.5,.82);L(.5,.82,.84,.65);L(.5,.30,.5,.64);break;
        case EAIHeroesGlyph::Wood:
            Ring(.27,.7,.17,.17);L(.12,.59,.59,.12);L(.41,.82,.89,.35);
            L(.59,.12,.89,.35);L(.33,.55,.69,.20);L(.43,.66,.79,.30);break;
        case EAIHeroesGlyph::Ore:
            L(.10,.68,.26,.30);L(.26,.30,.60,.15);L(.60,.15,.88,.44);
            L(.88,.44,.83,.80);L(.83,.80,.36,.9);L(.36,.9,.10,.68);
            L(.26,.30,.45,.58);L(.45,.58,.88,.44);L(.45,.58,.36,.9);break;
        case EAIHeroesGlyph::Data:
            L(.5,.06,.80,.38);L(.8,.38,.66,.83);L(.66,.83,.38,.96);
            L(.38,.96,.18,.54);L(.18,.54,.5,.06);L(.5,.06,.5,.62);
            L(.18,.54,.5,.62);L(.8,.38,.5,.62);L(.5,.62,.38,.96);break;
        case EAIHeroesGlyph::Castle:
            L(.14,.87,.14,.35);L(.14,.35,.34,.35);L(.34,.35,.34,.65);
            L(.34,.65,.66,.65);L(.66,.65,.66,.35);L(.66,.35,.86,.35);
            L(.86,.35,.86,.87);L(.14,.87,.86,.87);L(.38,.65,.38,.20);
            L(.38,.2,.62,.2);L(.62,.2,.62,.65);L(.48,.87,.48,.72);
            L(.14,.35,.14,.21);L(.86,.35,.86,.21);L(.5,.2,.5,.06);break;
        case EAIHeroesGlyph::Hero:
            L(.2,.7,.2,.35);L(.2,.35,.5,.10);L(.5,.10,.8,.35);
            L(.8,.35,.8,.7);L(.2,.7,.4,.88);L(.8,.7,.6,.88);
            L(.4,.88,.4,.47);L(.6,.88,.6,.47);L(.2,.43,.4,.47);
            L(.8,.43,.6,.47);L(.5,.10,.5,.38);break;
        case EAIHeroesGlyph::Army:
            Ring(.5,.27,.13,.15);Ring(.19,.41,.10,.12);Ring(.81,.41,.10,.12);
            L(.29,.84,.32,.51);L(.32,.51,.68,.51);L(.68,.51,.71,.84);
            L(.29,.84,.71,.84);L(.1,.6,.08,.84);L(.08,.84,.23,.84);
            L(.9,.6,.92,.84);L(.92,.84,.77,.84);break;
        case EAIHeroesGlyph::Scroll:
            L(.22,.12,.76,.12);L(.76,.12,.8,.24);L(.8,.24,.68,.24);
            L(.68,.24,.68,.87);L(.68,.87,.18,.87);L(.18,.87,.12,.74);
            L(.12,.74,.25,.74);L(.25,.74,.25,.12);
            for(float Y:{.35f,.49f,.63f})L(.34,Y,.59,Y);break;
        case EAIHeroesGlyph::Hourglass:
            L(.2,.1,.8,.1);L(.2,.9,.8,.9);L(.25,.1,.25,.29);
            L(.75,.1,.75,.29);L(.25,.29,.75,.71);L(.75,.29,.25,.71);
            L(.25,.71,.25,.9);L(.75,.71,.75,.9);L(.32,.82,.68,.82);break;
        case EAIHeroesGlyph::Sun:
            Ring(.5,.5,.22,.22);
            for(int I=0;I<8;++I){const double A=I*PI/4;L(.5+.33*FMath::Cos(A),.5+.33*FMath::Sin(A),.5+.46*FMath::Cos(A),.5+.46*FMath::Sin(A));}break;
        case EAIHeroesGlyph::Compass:
            Ring(.5,.5,.31,.31);L(.5,.02,.64,.5);L(.64,.5,.5,.98);
            L(.5,.98,.36,.5);L(.36,.5,.5,.02);L(.02,.5,.98,.5);break;
        default: break;
    }
    return Out;
}
FVector2D FAIHeroesReferenceUI::Project(const FVector& P)
{
    // Fixed public board bounds, not bounds inferred from unseen objects.
    return {FMath::Clamp((P.Y+1700.0)/3400.0,0.0,1.0),
        FMath::Clamp((1700.0-P.X)/3400.0,0.0,1.0)};
}
FAIHeroesOverview FAIHeroesReferenceUI::Overview(const FAIHeroesAdvisorSnapshot& S)
{
    FAIHeroesOverview Out;
    if(S.Nodes.Num()!=5 || S.HeroPosition.ContainsNaN() ||
        !FAIHeroesTravelRules::IsSupported(S.TravelRuleVersion))return Out;
    FAIHeroesAdventureGraph Graph;FString Error;
    // Validate topology with the SAME road authority, never connect by distance.
    FAIHeroesAdvisorSnapshot Roads=S;Roads.TravelRuleVersion=FAIHeroesTravelRules::RoadVersion;
    if(!FAIHeroesTravelRules::BuildGraph(Roads,Graph,Error))return Out;
    TSet<FName> Owned;FName Origin;
    for(const auto& N:S.Nodes)
    {
        if(N.bOwned)Owned.Add(N.Id);
        if(FVector::DistSquared(N.Location,S.HeroPosition)<1.0)Origin=N.Id;
    }
    if(!FAIHeroesFogRules::Validate(S.bFogEnabled,S.ExploredSitesMask,Origin,S.SelectedNodeId,Owned,Error))return Out;
    if(Origin.IsNone())return Out;
    for(const auto& N:S.Nodes)
    {
        if(!FAIHeroesFogRules::IsKnown(S.bFogEnabled,S.ExploredSitesMask,N.Id))continue;
        FAIHeroesOverviewMark M;M.Id=N.Id;M.Type=N.Type;M.UV=Project(N.Location);M.bOwned=N.bOwned;
        for(const auto& U:N.Guards)M.bGuarded|=U.IsAlive();
        Out.Marks.Add(M);
    }
    Out.Marks.Sort([](const auto& A,const auto& B){return A.Id.LexicalLess(B.Id);});
    if(S.TravelRuleVersion==FAIHeroesTravelRules::RoadVersion)
    for(const auto& L:Graph.Links)
    {
        const int32 A=Out.Marks.IndexOfByPredicate([&](const auto& M){return M.Id==L.From;});
        const int32 B=Out.Marks.IndexOfByPredicate([&](const auto& M){return M.Id==L.To;});
        if(A!=INDEX_NONE&&B!=INDEX_NONE&&A<B)Out.Links.Add({A,B});
    }
    Out.HeroUV=Project(S.HeroPosition);Out.bValid=true;return Out;
}
