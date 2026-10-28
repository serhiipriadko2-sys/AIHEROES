# AI HEROES — Campaign Objectives V1: local QA receipt

Дата: 2026-10-28. Проверяемая поверхность: Windows, `C:\github\AIHEROES`.
Промежуточный lifecycle: **implemented → compiled → automated-tested → visual-smoke-tested locally**.
Ни commit/push, ни packaged Android APK, ни релиз, ни реальная 1B/3B/8B LLM не заявлены.

## Intake / SIFT

- Источник игры: текущий Unreal Engine 5.8 C++ проект, активный `AAIHeroesGameMode`.
- Сверка локальной SoT30 v5.5.7: `SOT30_INTEGRITY_PASS`,
  Knowledge 30/30, SHA256SUMS 32/32, 4 054 425 байт.
- Truth: `AIHeroesCampaignObjectives` — **игровые миссии**. Это не
  Iskra Memory, не фактическая смена нейросети и не разрешение ИИ
  выполнять стратегические команды.
- Существующие локальные правки HQ/AI Proposal/Grid Replay других
  разработчиков сохранены. Эта QA не коммитила и не делала push.

## Реализация / Frame

Введены 4 независимых задания: FOREST+ORE, OUTPOST+RUINS, 3 офлайн-протокола,
тематический слот 8B. Финал только при 4/4. Дубликаты ID/подмены типов
не засчитываются. Победа всегда вычисляется из уже сохраняемых полей
`NodeStates, ProtocolTier, ModelSlotTier`: миграция SaveGame не нужна.

Интерфейс: полоса `CAMPAIGN 0/4 | NEXT: Capture FOREST`,
сенсорная кнопка `GOALS O` и клавиша O; модальное досье с целями,
страницами и Refresh. При открытом досье приказ на следующий день
заблокирован. Обновление досье не тратит ресурсы и не даёт игровых
приказов. На достижении 4/4 показывается `CAMPAIGN VICTORY`,
игру можно продолжить или сохранить вручную через K.

## Верификация / Verify

| Gate | Итог | Evidence |
|---|---|---|
| UBT Editor компиляция | PASS | `AIHeroesEditor Win64 Development`; DLL после C++-исходников |
| `AIHeroes.Objectives.CampaignProgression` | PASS | свежий полный Unreal JSON |
| `AIHeroes.Objectives.DerivedFromSaveGame` | PASS | переход старой SaveGame версии без новых полей |
| `AIHeroes.Objectives.DuplicateAndSpoofedNodesRejected` | PASS | ID/тип/владелец |
| `AIHeroes.Objectives.TouchMissionButton` | PASS | 1280×720 и 480×800; во время боя неактивна |
| Полный Unreal Automation | **77/77 PASS**, 0 Fail, 0 Warn, 0 NotRun | `Docs/AutomationRuns/20261028T200750062Z/index.json` |
| Fresh source vs DLL | PASS | `SOURCE_NEWER_THAN_DLL=0` |
| Реальное отдельное окно игры | PASS | `-log=AIHeroesObjectivesQA.log` |
| O открывает досье с 4 целями | PASS | `[AI HEROES OBJECTIVES] OPEN 0/4 victory=0 no_order=1` |
| Space на открытом досье | PASS (заблокирован) | `REPORT OPEN: close the read-only dossier before giving orders` |
| O закрывает досье | PASS | `[AI HEROES DOSSIER] CLOSE no_order=1` |
| Непреднамеренные save/load в QA | 0 | `OBJECTIVES_MODAL_INPUT_PASS ... no_save=1` |
| Repo-artifact inventory | PASS на момент проверки | 242 файла, 15 511 358 байт; нет symlink/case collisions/secret-like paths |
| Реальное появление 4/4 при длительной кампании | НЕ ПРОВЕРЕНО вручную | unit test проверяет состояние 4/4 |
| Android APK / настоящий LLM | НЕ РЕАЛИЗОВАНО | отдельные будущие gates |

На начальном запуске `AIHeroes.Objectives` тестовая программа аварийно
остановилась из-за `Input.Nodes.Add(Input.Nodes[1])`: Unreal запрещает
передавать ссылку на собственный `TArray` при его расширении.
Исправлено на независимую копию. После этого в параллельной разработке
отдельно исправлена регрессия V1 Grid Replay; свежий **полный** прогон
77/77 является окончательным. Первые неуспешные прогоны не считаются PASS.

## Доказательные артефакты

| Путь относительно корня | Байты | SHA256 |
|---|---:|---|
| `Source/AIHeroes/Public/AIHeroesCampaignObjectives.h` | 1298 | `4500061EA7FAA509E5616AA8422269147D14EB6A38836D4C3ED60B0D1E9433BE` |
| `Source/AIHeroes/Private/AIHeroesCampaignObjectives.cpp` | 4121 | `CE483F16E3BA2EE4F8B0D1C3E10B9F72EB1D4F293E5917E18F9EDA835083BB37` |
| `Source/AIHeroes/Private/AIHeroesCampaignObjectivesGameMode.cpp` | 1917 | `637C6357BE97D4348C1DCB8F8D09C34B3390F51336C7BB9E3CE5C375287ABCAA` |
| `Source/AIHeroes/Private/Tests/AIHeroesCampaignObjectivesTests.cpp` | 8531 | `8AD508A5C3A71131839112F273E93BE78CC61A475E7C18BB9F94B71C9C2E80A4` |
| `Docs/CAMPAIGN_OBJECTIVES_V1.md` | 5603 | `321F6E9B43A578E555352E81EDCCEC49DC935F5DD0E405C84B7BDEFF0D8E5823` |
| `Tools/qa_campaign_objectives.ps1` | 2626 | `6E455D2522B4EE4FFF159F30860BDFE60687ECD17C6200411B1A59D7802D6863` |
| `Docs/MISSION_OBJECTIVES_START_2026-10-28.png` | 357679 | `A972B47482FA8C6B97CB3AE3B3769B4021018773A1B94A016E62CDF1E22EABFE` |
| `Docs/MISSION_OBJECTIVES_DOSSIER_2026-10-28.png` | 87942 | `D0F90D4DFCDC27A2938909C183F6DADE46B2338543F44478471BBF758883917C` |
| `Docs/MISSION_OBJECTIVES_RETURN_2026-10-28.png` | 356367 | `E0F38FAF29C3011609FA78E0134AE353D0D7298AA35B77F63DF5E4EF5231948B` |
| `Docs/AutomationRuns/20261028T200750062Z/index.json` | 30482 | `1CA37AB38B338D582E1013FDB29D24C7EDD3ED873B8B54BB4A8831775C38C15B` |

Дополнительная инвентаризация: `Saved/QA/repo_manifest.json`,
63 294 байта, SHA256
`137bd00fb9bd14e426f6b20fcdf1b772c05fc1ae79205ed879f0c2c734aa7c91`.
Только локальная верификация, не release receipt.

## Остаточные границы и следующий этап

Карта содержит лишь 5 прототипных узлов, настоящие NPC-стратегии,
большая карта, художественные ресурсы, Android build и LLM-инференс
пока не реализованы. Баланс стоимости протоколов/HQ ещё не подтверждён
плейтестами. Система целей не выдаёт наград, которые нельзя восстановить
из SaveGame, и не включает автоматического сохранения победы.

Следующий осмысленный игровой шаг: противоборствующая фракция на карте,
стратегические действия NPC и события между ходами (с проверяемой
детерминированностью), затем художественный 3D-пайплайн и Android device QA.

**Статус: PASS локальная система целей и PC-QA / PARTIAL вся игра.**

∆DΩΛ: ∆ — добавлена конечная цель кампании; D — исходники, тесты 77/77,
игровые скриншоты, JSON, SHA256; Ω — 94% для локально проверенной версии;
Λ — перепроверить после новых коммитов/плейтестов/Android SDK.
