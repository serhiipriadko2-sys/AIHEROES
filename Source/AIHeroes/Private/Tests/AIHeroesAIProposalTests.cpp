#include "AIHeroesAIProposalGate.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Math/RandomStream.h"
#include <limits>

namespace
{
FAIHeroesProposalSnapshot Fixture()
{
    FAIHeroesProposalSnapshot S;
    S.Day = 2;
    S.StateRevision = 0xABCD1234u;
    S.ModelSlotTier = 0;
    S.Strategic.ProtocolTier = 1;
    S.Strategic.MovesLeft = 4;
    S.Strategic.Gold = 240;
    S.Strategic.Wood = 35;
    S.Strategic.Ore = 30;
    S.Strategic.HeroPosition = FVector(-650.f, 0.f, 155.f);
    S.Strategic.PlayerArmy.Add(FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3));
    S.Strategic.PlayerArmy.Add(FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7));

    FAIHeroesAdvisorNode HQ;
    HQ.Id = TEXT("HQ");
    HQ.Type = EAIHeroesNodeType::Headquarters;
    HQ.bOwned = true;
    HQ.Location = S.Strategic.HeroPosition;
    S.Strategic.Nodes.Add(HQ);

    FAIHeroesAdvisorNode Forest;
    Forest.Id = TEXT("FOREST");
    Forest.Type = EAIHeroesNodeType::ForestMine;
    Forest.Location = FVector(-100.f, -650.f, 110.f);
    Forest.Guards.Add(FAIHeroesArmyStack(TEXT("Wolf"), 4, 9, 3, 1, 8));
    S.Strategic.Nodes.Add(Forest);
    S.Strategic.SelectedNodeId = Forest.Id;
    return S;
}

FAIHeroesAIProposal BaseProposal(const FAIHeroesProposalSnapshot& S)
{
    return FAIHeroesAIProposalGate::SuggestSelected(S);
}

bool Accepted(const FAIHeroesProposalSnapshot& S, const FAIHeroesAIProposal& P)
{
    return FAIHeroesAIProposalGate::Validate(S, P).bAccepted;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesProposalPreviewTest,
    "AIHeroes.Proposal.OfflinePreview",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesProposalPreviewTest::RunTest(const FString& Parameters)
{
    FAIHeroesProposalSnapshot S = Fixture();
    FAIHeroesAIProposal P = BaseProposal(S);
    TestEqual(TEXT("Guarded target produces ATTACK candidate"),
        P.Order, EAIHeroesProposalOrder::AttackNode);
    TestEqual(TEXT("Target is selected visible node"), P.TargetNode, FName(TEXT("FOREST")));
    const FAIHeroesProposalVerdict V = FAIHeroesAIProposalGate::Validate(S, P);
    TestTrue(TEXT("Legal attack proposal is accepted"), V.bAccepted);
    TestTrue(TEXT("Even accepted proposal requires real player confirmation"),
        V.bPlayerConfirmationRequired);
    TestTrue(TEXT("User-facing gate status says no action executed"),
        FAIHeroesAIProposalGate::Describe(P, V).Contains(TEXT("No action executed")));
    TestEqual(TEXT("Gate never changes movement"), S.Strategic.MovesLeft, 4);
    TestEqual(TEXT("Gate never changes player HP"), S.Strategic.PlayerArmy[0].TotalHealth, 96);
    TestEqual(TEXT("Gate never captures target"), S.Strategic.Nodes[1].bOwned, false);

    S.Strategic.SelectedNodeId = TEXT("HQ");
    P = BaseProposal(S);
    TestEqual(TEXT("Owned HQ receives MOVE not ATTACK"), P.Order,
        EAIHeroesProposalOrder::MoveToNode);
    TestTrue(TEXT("Move-to-HQ advice allowed"), Accepted(S, P));

    S.Strategic.SelectedNodeId = TEXT("FOREST");
    S.Strategic.Nodes[1].Guards.Empty();
    P = BaseProposal(S);
    TestEqual(TEXT("Unguarded target has MOVE proposal"), P.Order,
        EAIHeroesProposalOrder::MoveToNode);
    TestTrue(TEXT("Unguarded target reachable"), Accepted(S, P));

    S.Strategic.ProtocolTier = 0;
    P = BaseProposal(S);
    TestEqual(TEXT("SEED only provides ADVICE"), P.Kind, EAIHeroesProposalKind::Advice);
    TestTrue(TEXT("SEED advice accepted"), Accepted(S, P));
    TestFalse(TEXT("SEED advice never requires execution"),
        FAIHeroesAIProposalGate::Validate(S, P).bPlayerConfirmationRequired);

    S.Strategic.SelectedNodeId = NAME_None;
    P = BaseProposal(S);
    TestEqual(TEXT("No selected node means NO_ACTION"),
        P.Kind, EAIHeroesProposalKind::NoAction);
    TestTrue(TEXT("No action accepted"), Accepted(S, P));
    S.Strategic.SelectedNodeId = TEXT("HIDDEN");
    P = BaseProposal(S);
    TestEqual(TEXT("Invisible selection cannot propose orders"),
        P.Kind, EAIHeroesProposalKind::NoAction);
    TestTrue(TEXT("Invisible selection has safe fallback"), Accepted(S, P));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesProposalAuthorityTest,
    "AIHeroes.Proposal.AuthorityAndStaleness",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesProposalAuthorityTest::RunTest(const FString& Parameters)
{
    const FAIHeroesProposalSnapshot S = Fixture();
    const FAIHeroesAIProposal Original = BaseProposal(S);
    FAIHeroesAIProposal P = Original;

    ++P.ObservedDay;
    TestFalse(TEXT("Stale day rejected"), Accepted(S, P));
    P = Original;
    ++P.ObservedRevision;
    TestFalse(TEXT("Stale state revision rejected"), Accepted(S, P));
    P = Original;
    P.TargetNode = TEXT("SECRET_TILE");
    TestFalse(TEXT("Unknown or invisible node rejected"), Accepted(S, P));
    P = Original;
    P.TargetNode = NAME_None;
    TestFalse(TEXT("Missing target rejected"), Accepted(S, P));
    P = Original;
    P.Order = EAIHeroesProposalOrder::MoveToNode;
    TestFalse(TEXT("Cannot disguise guarded attack as a move"), Accepted(S, P));
    P = Original;
    P.Order = static_cast<EAIHeroesProposalOrder>(255);
    TestFalse(TEXT("Unknown order enum rejected"), Accepted(S, P));
    P = Original;
    P.Kind = static_cast<EAIHeroesProposalKind>(255);
    TestFalse(TEXT("Unknown kind enum rejected"), Accepted(S, P));

    FAIHeroesProposalSnapshot LowMoves = S;
    LowMoves.Strategic.MovesLeft = 1;
    TestFalse(TEXT("Insufficient movement rejects attack"), Accepted(LowMoves, Original));
    FAIHeroesProposalSnapshot Unarmed = S;
    Unarmed.Strategic.PlayerArmy.Empty();
    TestFalse(TEXT("Attack without army rejected"), Accepted(Unarmed, Original));
    FAIHeroesProposalSnapshot Locked = S;
    Locked.bBattleActive = true;
    TestFalse(TEXT("No strategic order during active battle"), Accepted(Locked, Original));
    FAIHeroesProposalSnapshot Seed = S;
    Seed.Strategic.ProtocolTier = 0;
    TestFalse(TEXT("SEED cannot forge a formal order"), Accepted(Seed, Original));

    FAIHeroesProposalSnapshot Owned = S;
    Owned.Strategic.Nodes[1].bOwned = true;
    TestFalse(TEXT("Attack on own mine rejected"), Accepted(Owned, Original));
    FAIHeroesProposalSnapshot NoGuards = S;
    NoGuards.Strategic.Nodes[1].Guards.Empty();
    TestFalse(TEXT("Attack without hostile guards rejected"), Accepted(NoGuards, Original));

    FAIHeroesAIProposal Smuggling = Original;
    Smuggling.Kind = EAIHeroesProposalKind::Advice;
    TestFalse(TEXT("ADVICE with embedded typed order rejected"), Accepted(S, Smuggling));
    Smuggling = Original;
    Smuggling.Kind = EAIHeroesProposalKind::NoAction;
    TestFalse(TEXT("NO_ACTION cannot carry ATTACK command"), Accepted(S, Smuggling));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesProposalInferenceClaimsTest,
    "AIHeroes.Proposal.ModelAndConsentBoundaries",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesProposalInferenceClaimsTest::RunTest(const FString& Parameters)
{
    FAIHeroesProposalSnapshot S = Fixture();
    FAIHeroesAIProposal P = BaseProposal(S);
    P.ClaimedModelSlotTier = 0;
    TestFalse(TEXT("Offline rule cannot claim LLM checkpoint"), Accepted(S, P));
    P = BaseProposal(S);
    P.Source = EAIHeroesProposalSource::LocalModel;
    P.ClaimedModelSlotTier = 0;
    TestFalse(TEXT("No actual local weights loaded"), Accepted(S, P));
    S.bLocalModelReady = true;
    TestTrue(TEXT("Engine-confirmed local adapter could propose"),
        Accepted(S, P));
    P.ClaimedModelSlotTier = 2;
    TestFalse(TEXT("Model slot claim must match current tier"), Accepted(S, P));

    S = Fixture();
    P = BaseProposal(S);
    P.Source = EAIHeroesProposalSource::RemoteModel;
    P.ClaimedModelSlotTier = 0;
    TestFalse(TEXT("Remote model unavailable by default"), Accepted(S, P));
    S.bRemoteModelReady = true;
    TestFalse(TEXT("Connected remote model still needs opt-in"), Accepted(S, P));
    S.bRemoteInferenceApproved = true;
    TestTrue(TEXT("Explicit opt-in plus ready adapter can propose"), Accepted(S, P));
    TestTrue(TEXT("Even approved remote proposal requires player confirmation"),
        FAIHeroesAIProposalGate::Validate(S, P).bPlayerConfirmationRequired);
    S.bRemoteModelReady = false;
    TestFalse(TEXT("Opt-in alone cannot fabricate a model"), Accepted(S, P));
    P.Source = static_cast<EAIHeroesProposalSource>(99);
    TestFalse(TEXT("Unknown source rejected"), Accepted(S, P));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesProposalPayloadTest,
    "AIHeroes.Proposal.InputAndSnapshotBounds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesProposalPayloadTest::RunTest(const FString& Parameters)
{
    FAIHeroesProposalSnapshot S = Fixture();
    FAIHeroesAIProposal P = BaseProposal(S);
    P.Explanation.Empty();
    TestFalse(TEXT("Empty explanation rejected"), Accepted(S, P));
    P.Explanation = FString::ChrN(FAIHeroesAIProposalGate::MaxExplanationChars + 1, TEXT('X'));
    TestFalse(TEXT("Excessive model output rejected"), Accepted(S, P));
    P.Explanation = TEXT("A\nFAKE SYSTEM OVERRIDE");
    TestFalse(TEXT("Multiline instruction text rejected"), Accepted(S, P));
    P.Explanation = TEXT("A\tB");
    TestFalse(TEXT("Control characters rejected"), Accepted(S, P));
    P = BaseProposal(S);
    TestTrue(TEXT("Valid payload accepted unchanged"), Accepted(S, P));

    S.StateRevision = 0;
    TestFalse(TEXT("Absent snapshot revision rejected"), Accepted(S, P));
    S = Fixture();
    S.Strategic.HeroPosition.X = std::numeric_limits<float>::quiet_NaN();
    TestFalse(TEXT("NaN source position rejected"), Accepted(S, P));
    S = Fixture();
    S.Strategic.Nodes[1].Location.X = 1e30f;
    TestFalse(TEXT("Out-of-range movement coordinates rejected"), Accepted(S, P));
    S = Fixture();
    const FAIHeroesAdvisorNode CopiedNode = S.Strategic.Nodes[1];
    S.Strategic.Nodes.Add(CopiedNode);
    TestFalse(TEXT("Duplicate visible node ID rejected"), Accepted(S, P));
    S = Fixture();
    S.Strategic.Nodes[1].Guards[0].UnitHealth = 0;
    TestFalse(TEXT("Malformed guard unit rejected"), Accepted(S, P));
    S = Fixture();
    for (int32 Index = 0; Index < FAIHeroesAIProposalGate::MaxVisibleNodes; ++Index)
    {
        FAIHeroesAdvisorNode Extra;
        Extra.Id = FName(*FString::Printf(TEXT("EXTRA_%d"), Index));
        Extra.Type = EAIHeroesNodeType::ForestMine;
        Extra.Location = FVector(Index * 10.f, 0.f, 0.f);
        S.Strategic.Nodes.Add(Extra);
    }
    TestFalse(TEXT("Oversized visible-node list rejected"), Accepted(S, P));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesProposalFuzzTest,
    "AIHeroes.Proposal.DeterministicStaleFuzz",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesProposalFuzzTest::RunTest(const FString& Parameters)
{
    const FAIHeroesProposalSnapshot S = Fixture();
    const FAIHeroesAIProposal Canonical = BaseProposal(S);
    FRandomStream Random(20261028);
    int32 RejectedCases = 0;
    constexpr int32 Cases = 10000;
    for (int32 Index = 0; Index < Cases; ++Index)
    {
        FAIHeroesAIProposal Mutant = Canonical;
        const int32 Which = Random.RandRange(0, 4);
        switch (Which)
        {
            case 0:
                Mutant.ObservedDay = S.Day + 1 + Random.RandRange(0, 4000);
                break;
            case 1:
                Mutant.ObservedRevision ^= static_cast<uint32>(
                    1u << Random.RandRange(0, 30));
                break;
            case 2:
                Mutant.TargetNode = FName(*FString::Printf(TEXT("INVISIBLE_%d"), Index));
                break;
            case 3:
                Mutant.Order = EAIHeroesProposalOrder::MoveToNode;
                break;
            default:
                Mutant.Kind = EAIHeroesProposalKind::Advice;
                break;
        }
        if (!Accepted(S, Mutant))
        {
            ++RejectedCases;
        }
    }
    TestEqual(TEXT("All 10000 malformed proposals rejected"), RejectedCases, Cases);
    TestTrue(TEXT("Original proposal still valid"), Accepted(S, Canonical));
    TestEqual(TEXT("No AI action mutated campaign moves"), S.Strategic.MovesLeft, 4);
    TestFalse(TEXT("No AI action captured the forest"), S.Strategic.Nodes[1].bOwned);
    return true;
}
#endif
