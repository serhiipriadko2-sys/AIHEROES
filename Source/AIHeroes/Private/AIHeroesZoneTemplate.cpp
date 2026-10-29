#include "AIHeroesZoneTemplate.h"

#include "AIHeroesZoneTemplateCore.h"

namespace
{
AIHeroesZoneTemplateCore::World AsPortable(const FAIHeroesGeneratedWorld& Draft)
{
    AIHeroesZoneTemplateCore::World W;
    W.Version = Draft.Schema;
    W.Seed = Draft.Seed;
    W.Revision = Draft.Revision;
    for (const FAIHeroesGeneratedZone& Z : Draft.Zones)
    {
        W.Zones.push_back({
            TCHAR_TO_UTF8(*Z.Id.ToString()),
            static_cast<AIHeroesZoneTemplateCore::Role>(Z.Role),
            Z.Grid.X, Z.Grid.Y, Z.DangerTier, Z.bGuarded
        });
    }
    for (const FAIHeroesAdventureLink& E : Draft.Adventure.Links)
        W.Roads.push_back({
            TCHAR_TO_UTF8(*E.From.ToString()), TCHAR_TO_UTF8(*E.To.ToString()),
            E.MoveCost
        });
    return W;
}
}

bool FAIHeroesZoneTemplate::Generate(
    uint32 Seed, int32 ExtraRoutes,
    FAIHeroesGeneratedWorld& OutDraft, FString& Error)
{
    OutDraft = FAIHeroesGeneratedWorld();
    Error.Empty();
    if (ExtraRoutes < 0 || ExtraRoutes > MaxExtraRoutes)
    {
        Error = TEXT("Unsupported number of additional passages.");
        return false;
    }
    AIHeroesZoneTemplateCore::World W;
    std::string Why;
    if (!AIHeroesZoneTemplateCore::Generate(
            Seed, static_cast<uint32>(ExtraRoutes), W, Why))
    {
        Error = UTF8_TO_TCHAR(Why.c_str());
        return false;
    }

    FAIHeroesGeneratedWorld Proposed;
    Proposed.Schema = W.Version;
    Proposed.Seed = W.Seed;
    Proposed.Revision = W.Revision;
    Proposed.Adventure.Revision = W.Revision;
    for (const auto& Z : W.Zones)
    {
        FAIHeroesGeneratedZone Result;
        Result.Id = FName(UTF8_TO_TCHAR(Z.Id.c_str()));
        Result.Role = static_cast<EAIHeroesZoneRole>(Z.Type);
        Result.Grid = FIntPoint(Z.X, Z.Y);
        Result.DangerTier = Z.Danger;
        Result.bGuarded = Z.Guarded;
        Proposed.Zones.Add(Result);
        Proposed.Adventure.Nodes.Add({Result.Id, true, true, Result.bGuarded});
    }
    for (const auto& E : W.Roads)
    {
        Proposed.Adventure.Links.Add({
            FName(UTF8_TO_TCHAR(E.From.c_str())),
            FName(UTF8_TO_TCHAR(E.To.c_str())),
            E.Cost, true
        });
    }

    if (!Validate(Proposed, Error)) return false;
    OutDraft = MoveTemp(Proposed);
    return true;
}

bool FAIHeroesZoneTemplate::Validate(
    const FAIHeroesGeneratedWorld& Draft, FString& Error)
{
    Error.Empty();
    if (Draft.Zones.Num() < static_cast<int32>(AIHeroesZoneTemplateCore::BaseZones) ||
        Draft.Zones.Num() > static_cast<int32>(AIHeroesZoneTemplateCore::MaxZones) ||
        Draft.Adventure.Nodes.Num() != Draft.Zones.Num() ||
        Draft.Adventure.Revision != Draft.Revision ||
        Draft.Adventure.Links.Num() > 48)
    {
        Error = TEXT("Untrusted dynamic map is incorrectly bounded.");
        return false;
    }
    for (int32 I = 0; I < Draft.Zones.Num(); ++I)
    {
        const auto& Z = Draft.Zones[I];
        const auto& N = Draft.Adventure.Nodes[I];
        if (N.Id != Z.Id || N.bHostileGuard != Z.bGuarded ||
            !N.bDiscovered || !N.bPassable)
        {
            Error = TEXT("Graph node and zone metadata disagree.");
            return false;
        }
    }
    for (const auto& Link : Draft.Adventure.Links)
    {
        if (!Link.bEnabled)
        {
            Error = TEXT("Dynamic map contains an unapproved blocked connection.");
            return false;
        }
    }
    const auto W = AsPortable(Draft);
    std::string Why;
    if (!AIHeroesZoneTemplateCore::Validate(W, Why))
    {
        Error = UTF8_TO_TCHAR(Why.c_str());
        return false;
    }
    return true;
}

FString FAIHeroesZoneTemplate::Describe(const FAIHeroesGeneratedWorld& Draft)
{
    FString Reason;
    if (!Validate(Draft, Reason))
        return TEXT("FUTURE MAP DRAFT INVALID: ") + Reason;
    return FString::Printf(
        TEXT("FUTURE MAP DRAFT ONLY | seed=%u | zones=%d | directed roads=%d | revision=%u | two early guarded branches, late locked gate; NOT ACTIVE CAMPAIGN"),
        Draft.Seed, Draft.Zones.Num(), Draft.Adventure.Links.Num(), Draft.Revision);
}
