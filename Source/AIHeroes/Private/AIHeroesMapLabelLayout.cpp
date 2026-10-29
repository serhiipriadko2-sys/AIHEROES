#include "AIHeroesMapLabelLayout.h"

namespace MapLabelLayoutDetail
{
bool ValidRect(const FAIHeroesHUDActionRect& R)
{
    return FMath::IsFinite(R.X) && FMath::IsFinite(R.Y) &&
        FMath::IsFinite(R.W) && FMath::IsFinite(R.H) &&
        R.W > 0.f && R.H > 0.f && FMath::Abs(R.X) <= 32768.f &&
        FMath::Abs(R.Y) <= 32768.f && R.W <= 32768.f && R.H <= 32768.f;
}
}

FAIHeroesHUDActionRect FAIHeroesMapLabelLayout::MapBounds(const FVector2D& V)
{
    FAIHeroesHUDActionRect R;
    if (!FMath::IsFinite(V.X) || !FMath::IsFinite(V.Y) ||
        V.X < 320.0 || V.Y < 240.0 || V.X > 16384.0 || V.Y > 16384.0) return R;
    const auto Goals = FAIHeroesHUDLayout::Rect(202, V);
    const float Bottom = Goals.H > 0.f ? Goals.Y - 8.f :
        static_cast<float>(V.Y) - FAIHeroesHUDLayout::BottomPanelHeight(V.X) - 8.f;
    // Header ends at 175; threat strip occupies 179..204. Keep 8 px clear.
    R.X = 12.f; R.Y = 212.f;
    R.W = static_cast<float>(V.X) - 24.f; R.H = Bottom - R.Y;
    return R.H >= 28.f ? R : FAIHeroesHUDActionRect();
}

bool FAIHeroesMapLabelLayout::ContainsRect(
    const FAIHeroesHUDActionRect& A, const FAIHeroesHUDActionRect& B)
{
    return MapLabelLayoutDetail::ValidRect(A) && MapLabelLayoutDetail::ValidRect(B) &&
        B.X >= A.X && B.Y >= A.Y && B.X + B.W <= A.X + A.W && B.Y + B.H <= A.Y + A.H;
}

bool FAIHeroesMapLabelLayout::Overlaps(
    const FAIHeroesHUDActionRect& A, const FAIHeroesHUDActionRect& B, float Gap)
{
    if (!MapLabelLayoutDetail::ValidRect(A) || !MapLabelLayoutDetail::ValidRect(B) ||
        !FMath::IsFinite(Gap)) return false;
    Gap = FMath::Clamp(Gap, 0.f, 64.f);
    return A.X < B.X + B.W + Gap && A.X + A.W + Gap > B.X &&
        A.Y < B.Y + B.H + Gap && A.Y + A.H + Gap > B.Y;
}

TArray<FAIHeroesMapLabelPlacement> FAIHeroesMapLabelLayout::Place(
    const FVector2D& V, const TArray<FAIHeroesMapLabelRequest>& Requests)
{
    TArray<FAIHeroesMapLabelPlacement> Out;
    const auto Safe = MapBounds(V);
    if (Safe.W < 150.f || Safe.H < 28.f || Requests.Num() > MaxLabels) return Out;
    TArray<FAIHeroesMapLabelRequest> Visible;
    TSet<FName> Seen;
    for (const auto& R : Requests)
    {
        if (!R.bKnown) continue;
        if (R.Id.IsNone() || R.Id.ToString().Len() > 64 || Seen.Contains(R.Id)) return {};
        Seen.Add(R.Id);
        if (!FMath::IsFinite(R.Anchor.X) || !FMath::IsFinite(R.Anchor.Y) ||
            R.Anchor.X < 0.0 || R.Anchor.X > V.X || R.Anchor.Y < 0.0 || R.Anchor.Y > V.Y) continue;
        Visible.Add(R);
    }
    Visible.Sort([](const auto& A, const auto& B)
    {
        if (A.bSelected != B.bSelected) return A.bSelected;
        return A.Id.ToString().ToLower() < B.Id.ToString().ToLower();
    });
    for (const auto& R : Visible)
    {
        const bool Detail = R.bHasDetail && Safe.H >= 52.f;
        const float Height = Detail ? 52.f : 28.f;
        const float DesiredX = FMath::Clamp(static_cast<float>(R.Anchor.X) - 75.f,
            Safe.X, Safe.X + Safe.W - 150.f);
        const float DesiredY = FMath::Clamp(static_cast<float>(R.Anchor.Y) - 26.f,
            Safe.Y, Safe.Y + Safe.H - Height);
        FAIHeroesHUDActionRect Best;
        double BestDistance = TNumericLimits<double>::Max();
        // At most 45 candidates per label; no open-ended packing loop.
        for (int32 Col : {0, -1, 1, -2, 2})
        for (int32 Row : {0, -1, 1, -2, 2, -3, 3, -4, 4})
        {
            FAIHeroesHUDActionRect Candidate;
            Candidate.W = 150.f; Candidate.H = Height;
            Candidate.X = FMath::Clamp(DesiredX + Col * 156.f, Safe.X, Safe.X + Safe.W - Candidate.W);
            Candidate.Y = FMath::Clamp(DesiredY + Row * (Height + 6.f), Safe.Y, Safe.Y + Safe.H - Height);
            if (!ContainsRect(Safe, Candidate)) continue;
            bool Collision = false;
            for (const auto& P : Out) Collision |= Overlaps(Candidate, P.Rect);
            if (Collision) continue;
            const double Distance = FMath::Square(double(Candidate.X - DesiredX)) +
                FMath::Square(double(Candidate.Y - DesiredY));
            if (Distance < BestDistance)
            {
                Best = Candidate;
                BestDistance = Distance;
            }
        }
        if (Best.W <= 0.f) continue;
        FAIHeroesMapLabelPlacement P;
        P.Id = R.Id; P.Anchor = R.Anchor; P.Rect = Best; P.bShowDetail = Detail;
        Out.Add(P);
    }
    return Out;
}
