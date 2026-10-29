#include "AIHeroesBasePanelCopy.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

namespace
{
FAIHeroesBasePanelState SampleBase()
{
    FAIHeroesBasePanelState S;
    S.Gold = 1000;
    S.Wood = 200;
    S.Ore = 200;
    S.Data = 100;
    S.bAtSelectedOwnedHQ = true;
    return S;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesBasePanelCopyOffersTest,
    "AIHeroes.UI.BasePanelCopy.OfferStates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesBasePanelCopyOffersTest::RunTest(const FString& Parameters)
{
    const FAIHeroesBasePanelState S = SampleBase();
    const auto Offers = FAIHeroesBasePanelRules::Offers(S);
    TestEqual(TEXT("All existing purchase choices are presented"), Offers.Num(), 5);
    if (Offers.Num() != 5) return false;

    for (int32 Index = 0; Index < Offers.Num(); ++Index)
    {
        const auto Kind = static_cast<EAIHeroesBasePurchase>(Index);
        const FString Title = FAIHeroesBasePanelCopy::Title(
            Kind, S.BaseTier, S.ProtocolTier, S.ModelSlotTier);
        TestFalse(TEXT("The player sees a nonempty title"), Title.IsEmpty());
        TestFalse(TEXT("The presentation is distinct from rules copy"),
            Title == Offers[Index].Title);
        TestTrue(TEXT("The player sees a price in Russian"),
            FAIHeroesBasePanelCopy::Price(Offers[Index]).Contains(TEXT("зол.")));
        TestTrue(TEXT("The player sees an unambiguous action state"),
            FAIHeroesBasePanelCopy::Status(Offers[Index]).Contains(TEXT("ВЫБРАТЬ")));
    }

    TestTrue(TEXT("Militia reserve is displayed"),
        FAIHeroesBasePanelCopy::Detail(EAIHeroesBasePurchase::Militia,
            S.MilitiaReserve, S.ScoutReserve).Contains(TEXT("Запас: 10")));
    TestTrue(TEXT("Scouts are identified as ranged"),
        FAIHeroesBasePanelCopy::Detail(EAIHeroesBasePurchase::Scouts,
            S.MilitiaReserve, S.ScoutReserve).Contains(TEXT("Стрелки")));
    TestTrue(TEXT("Model slot never claims installed weights"),
        FAIHeroesBasePanelCopy::Detail(EAIHeroesBasePurchase::ModelSlot,
            S.MilitiaReserve, S.ScoutReserve).Contains(TEXT("без нейровесов")));

    FAIHeroesBasePanelState Max = S;
    Max.BaseTier = 3;
    Max.ProtocolTier = 3;
    Max.ModelSlotTier = 2;
    const auto MaxOffers = FAIHeroesBasePanelRules::Offers(Max);
    TestTrue(TEXT("Maximum HQ is not shown as upgradeable"),
        FAIHeroesBasePanelCopy::Title(EAIHeroesBasePurchase::Headquarters,
            Max.BaseTier, Max.ProtocolTier, Max.ModelSlotTier).Contains(TEXT("МАКСИМУМ")));
    TestEqual(TEXT("Maximum protocol is visibly unavailable"),
        FAIHeroesBasePanelCopy::Status(MaxOffers[3]), FString(TEXT("НЕДОСТУПНО")));
    TestEqual(TEXT("Maximum slot remains a game label"),
        FAIHeroesBasePanelCopy::Status(MaxOffers[4]), FString(TEXT("НЕДОСТУПНО")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesBasePanelCopyFailClosedTest,
    "AIHeroes.UI.BasePanelCopy.SafetyAndMessages",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesBasePanelCopyFailClosedTest::RunTest(const FString& Parameters)
{
    FAIHeroesBasePanelState S = SampleBase();
    const auto Offers = FAIHeroesBasePanelRules::Offers(S);
    if (Offers.Num() != 5) return false;
    const auto& Quote = Offers[0];
    TestTrue(TEXT("The known good HQ offer is purchasable"), Quote.CanBuy());

    const FString Before = Quote.Price();
    const FString Preview = FAIHeroesBasePanelCopy::Message(
        TEXT("AFTER PURCHASE: 1 G / 2 W / 3 O / 4 DATA"),
        Quote);
    TestEqual(TEXT("Preview uses exactly the GameMode-provided amounts"),
        Preview, FString(TEXT("После покупки: 1 зол. / 2 дер. / 3 руд. / 4 дан.")));
    TestEqual(TEXT("Formatting does not change the authoritative price"),
        Quote.Price(), Before);
    TestTrue(TEXT("Confirmation is clearly one-shot"),
        FAIHeroesBasePanelCopy::Message(
            TEXT("PURCHASE APPLIED ONCE. Select another card to continue."),
            Quote).Contains(TEXT("Покупка выполнена")));

    S.Gold = 0;
    const auto NoFunds = FAIHeroesBasePanelRules::Offers(S);
    TestFalse(TEXT("A zero-budget offer is not purchasable"), NoFunds[0].CanBuy());
    TestTrue(TEXT("Resource failure explains the denial"),
        FAIHeroesBasePanelCopy::Reason(NoFunds[0]).Contains(TEXT("Не хватает ресурсов")));

    S.Gold = -1;
    const auto Invalid = FAIHeroesBasePanelRules::Offers(S);
    TestFalse(TEXT("Invalid state remains blocked"), Invalid[0].CanBuy());
    TestTrue(TEXT("Invalid state does not look like a price issue"),
        FAIHeroesBasePanelCopy::Reason(Invalid[0]).Contains(TEXT("Неверные данные")));

    const FString Unknown = TEXT("NEW_BACKEND_BLOCKED: save corrupted.");
    TestEqual(TEXT("Unknown engine failures are never hidden"),
        FAIHeroesBasePanelCopy::Message(Unknown, Invalid[0]), Unknown);
    return true;
}
#endif
