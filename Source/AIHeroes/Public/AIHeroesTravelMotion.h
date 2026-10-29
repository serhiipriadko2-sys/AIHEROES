#pragma once

#include "CoreMinimal.h"

// View-only interpolation. No actor, campaign, movement budget or save authority.
class AIHEROES_API FAIHeroesTravelMotion
{
public:
    static constexpr int32 MaxPoints = 129;
    bool Begin(const TArray<FVector>& InPoints, double UnitsPerSecond = 700.0)
    {
        if (InPoints.Num() < 2 || InPoints.Num() > MaxPoints ||
            !FMath::IsFinite(UnitsPerSecond) || UnitsPerSecond <= 0.0 ||
            UnitsPerSecond > 100000.0) return false;
        double Length = 0.0;
        for (int32 I = 0; I < InPoints.Num(); ++I)
        {
            const FVector& P = InPoints[I];
            if (P.ContainsNaN() || FMath::Abs(P.X) > 1000000.0 ||
                FMath::Abs(P.Y) > 1000000.0 || FMath::Abs(P.Z) > 1000000.0) return false;
            if (I > 0) Length += FVector::Distance(InPoints[I - 1], P);
        }
        if (!FMath::IsFinite(Length) || Length <= 0.001) return false;
        Points = InPoints;
        TotalLength = Length;
        Duration = FMath::Clamp(Length / UnitsPerSecond, 0.35, 5.0);
        Elapsed = 0.0;
        return true;
    }

    bool Advance(double DeltaSeconds)
    {
        if (!FMath::IsFinite(DeltaSeconds) || DeltaSeconds < 0.0) return false;
        Elapsed = FMath::Min(Duration, Elapsed + FMath::Min(DeltaSeconds, Duration));
        return true;
    }
    bool IsPlaying() const { return Points.Num() > 1 && Elapsed < Duration; }
    void Finish() { Elapsed = Duration; }
    void Reset() { Points.Reset(); TotalLength = Duration = Elapsed = 0.0; }
    double GetDuration() const { return Duration; }
    double GetProgress() const { return Duration > 0.0 ? Elapsed / Duration : 1.0; }
    const TArray<FVector>& GetPoints() const { return Points; }
    FVector GetDestination() const { return Points.IsEmpty() ? FVector::ZeroVector : Points.Last(); }
    FVector Position() const
    {
        if (Points.IsEmpty()) return FVector::ZeroVector;
        if (!IsPlaying()) return Points.Last();
        double Remaining = TotalLength * GetProgress();
        for (int32 I = 1; I < Points.Num(); ++I)
        {
            const double Segment = FVector::Distance(Points[I - 1], Points[I]);
            if (Segment <= 0.001) continue;
            if (Remaining <= Segment)
                return FMath::Lerp(Points[I - 1], Points[I], Remaining / Segment);
            Remaining -= Segment;
        }
        return Points.Last();
    }

private:
    TArray<FVector> Points;
    double TotalLength = 0.0, Duration = 0.0, Elapsed = 0.0;
};
