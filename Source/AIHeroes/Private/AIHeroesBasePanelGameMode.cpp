#include "AIHeroesGameMode.h"
#include "AIHeroesBasePanel.h"

FAIHeroesBasePanelOffer AAIHeroesGameMode::GetBasePanelOffer(EAIHeroesBasePurchase Kind) const
{
    FAIHeroesBasePanelState S;
    S.Gold = Gold; S.Wood = Wood; S.Ore = Ore; S.Data = DataFragments;
    S.BaseTier = BaseTier; S.ProtocolTier = ProtocolTier; S.ModelSlotTier = ModelSlotTier;
    S.bAtSelectedOwnedHQ = IsAtSelectedOwnedHeadquarters();
    S.Army = PlayerArmy;
    const auto Offers = FAIHeroesBasePanelRules::Offers(S);
    const int32 Index = static_cast<int32>(Kind);
    return Offers.IsValidIndex(Index) ? Offers[Index] : FAIHeroesBasePanelOffer();
}

bool AAIHeroesGameMode::IsBasePanelVisible() const
{
    return bBasePanelOpen && !IsLiveBattleActive() && !IsBattleReplayActive();
}

void AAIHeroesGameMode::ToggleBasePanel()
{
    if (IsLiveBattleActive() || IsBattleReplayActive() || IsAdvisorDossierVisible()) return;
    if (bBasePanelOpen) { CloseBasePanel(); return; }
    bBasePanelOpen = true;
    PendingBaseQuote = FAIHeroesBasePanelOffer();
    PendingBaseFingerprint.Empty();
    BasePanelMessage = TEXT("Select a card to review it. Nothing is purchased until CONFIRM.");
    UE_LOG(LogTemp, Display, TEXT("[AI HEROES BASE PANEL] OPEN no_order=1"));
}

void AAIHeroesGameMode::CloseBasePanel()
{
    if (!bBasePanelOpen) return;
    bBasePanelOpen = false;
    PendingBaseQuote = FAIHeroesBasePanelOffer();
    PendingBaseFingerprint.Empty();
    BasePanelMessage.Empty();
    UE_LOG(LogTemp, Display, TEXT("[AI HEROES BASE PANEL] CLOSE pending_discarded=1"));
}

void AAIHeroesGameMode::PreviewBasePurchase(EAIHeroesBasePurchase Kind)
{
    if (!IsBasePanelVisible() || IsAdvisorDossierVisible()) return;
    PendingBaseQuote = GetBasePanelOffer(Kind);
    PendingBaseFingerprint.Empty();
    if (PendingBaseQuote.Kind == EAIHeroesBasePurchase::None)
    {
        BasePanelMessage = TEXT("Unknown purchase rejected. Nothing spent.");
        return;
    }
    if (!PendingBaseQuote.CanBuy())
    {
        BasePanelMessage = PendingBaseQuote.BlockReason;
        return;
    }
    const FString Evidence = CampaignFingerprint();
    if (Evidence.IsEmpty() || Evidence == TEXT("INVALID MAP"))
    {
        BasePanelMessage = TEXT("Invalid map. Purchase blocked.");
        return;
    }
    PendingBaseFingerprint = Evidence;
    BasePanelMessage = FString::Printf(TEXT("AFTER PURCHASE: %d G / %d W / %d O / %d DATA"),
        Gold - PendingBaseQuote.Gold, Wood - PendingBaseQuote.Wood,
        Ore - PendingBaseQuote.Ore, DataFragments - PendingBaseQuote.Data);
    UE_LOG(LogTemp, Display, TEXT("[AI HEROES BASE PANEL] PREVIEW kind=%d no_order=1 price=%s"),
        static_cast<int32>(Kind), *PendingBaseQuote.Price());
}

bool AAIHeroesGameMode::CanConfirmBasePurchase() const
{
    return IsBasePanelVisible() && !IsAdvisorDossierVisible() &&
        !PendingBaseFingerprint.IsEmpty() && PendingBaseFingerprint != TEXT("INVALID MAP") &&
        PendingBaseFingerprint == CampaignFingerprint() && PendingBaseQuote.CanBuy() &&
        PendingBaseQuote.SameQuote(GetBasePanelOffer(PendingBaseQuote.Kind));
}

void AAIHeroesGameMode::ConfirmBasePurchase()
{
    if (!IsBasePanelVisible()) return;
    if (!CanConfirmBasePurchase())
    {
        PendingBaseQuote = FAIHeroesBasePanelOffer();
        PendingBaseFingerprint.Empty();
        BasePanelMessage = TEXT("No current quote. Select a card again; nothing spent.");
        UE_LOG(LogTemp, Display, TEXT("[AI HEROES BASE PANEL] REJECT no_current_quote=1"));
        return;
    }
    const auto Kind = PendingBaseQuote.Kind;
    const FString Before = PendingBaseFingerprint;
    // Consume the quote BEFORE dispatch. Repeated confirm cannot buy twice.
    PendingBaseQuote = FAIHeroesBasePanelOffer();
    PendingBaseFingerprint.Empty();
    {
        // Only this validated player-confirmation call releases the UI lock.
        // Existing command preconditions still execute, including HQ position.
        TGuardValue<bool> ReleasePanelLock(bBasePanelOpen, false);
        switch (Kind)
        {
            case EAIHeroesBasePurchase::Headquarters: UpgradeHeadquarters(); break;
            case EAIHeroesBasePurchase::Militia: RecruitMilitia(); break;
            case EAIHeroesBasePurchase::Scouts: RecruitScouts(); break;
            case EAIHeroesBasePurchase::Protocol: UpgradeProtocol(); break;
            case EAIHeroesBasePurchase::ModelSlot: UpgradeModelSlot(); break;
            default: return;
        }
    }
    const bool bChanged = CampaignFingerprint() != Before;
    BasePanelMessage = bChanged ? TEXT("PURCHASE APPLIED ONCE. Select another card to continue.") : LastEvent;
    UE_LOG(LogTemp, Display, TEXT("[AI HEROES BASE PANEL] CONFIRM kind=%d applied=%d gold=%d wood=%d ore=%d data=%d"),
        static_cast<int32>(Kind), bChanged, Gold, Wood, Ore, DataFragments);
}
