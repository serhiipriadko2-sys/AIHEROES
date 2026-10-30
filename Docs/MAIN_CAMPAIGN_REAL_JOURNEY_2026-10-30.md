# AIHEROES — непрерывное прохождение основной кампании (native QA)

Дата: 2026-10-30. Прогон: `AIHeroes.Playability.RealCampaign.EndToEndVictoryAndSaveReadback`.

**Статус: PASS для игрового GameMode/автоматизации и дискового read-back; ручное управление/экранная победа в новой DLL не проверены.**

## Результат

Новый автоматизированный «игрок» запускает **реальный** `AAIHeroesGameMode::DispatchBeginPlay()` с его пятью исходными узлами, туманом, войсками, защитниками, ресурсами и Road V2. Никакие стартовые охранники, их HP, финансовые поля, протоколы и флаг победы не меняются тестом напрямую.

Подтверждённая хронология по движению и боевым результатам (из `run.log`):

| День | Действие через штатные методы | Результат |
|---|---|---|
| 1 | На старте подтверждены 2 покупки по 5 militia и 2 покупки по 2 scout; поход в FOREST | Лесопилка захвачена; 2 очка движения осталось; 2 DATA |
| 2 | Завершение дня, поход FOREST → HQ → ORE | Рудник захвачен; 4 DATA, 0 движений |
| 3 | Завершение дня, переход к своей FOREST через HQ | Законная остановка; 0 движений |
| 4 | Переход FOREST → OUTPOST, затем OUTPOST → RUINS | Башня и архив захвачены; 4 подтверждённых боя; 10 DATA |
| 4–10 | Покупка игровых протоколов и слотов из **реально заработанной** экономики, завершение дней при нехватке средств | К концу дня 10 протокол 3, слот 8B (tier 2), `GetCampaignProgress().bVictory=true` |

Итоговая строка Engine: `[AIHEROES E2E] REAL VICTORY day=10 gold=0 wood=55 ore=24 data=2 journal=4`. Отдельно `SAVE READBACK VERIFIED day=10 seq=1 nodes=5`.

### Авторизация и честность

- Покупки происходят через действующие `ToggleBasePanel → PreviewBasePurchase → CanConfirmBasePurchase → ConfirmBasePurchase → CloseBasePanel`. Цена списывается штатными правилами.
- Походы через `GetTravelPathTo` (доступность) → `SelectNode` → `ConfirmSelectedNode`. Нет обхода тумана, чтения скрытых узлов или прямой установки владения.
- Бои проходят через `FastFinishLiveBattle` и `GameMode::Tick`, с реальными итогами `LastBattleRecord.bVictory`, наградой DATA и `ReturnFromLiveBattle`. Быстрая прокрутка не создаёт отдельную симуляцию.
- Если путь больше четырёх движений, игрок выбирает только собственную открытую промежуточную точку, достижимую по штатному поиску пути. Если на 4-х ходов не хватает, использует `EndTurn`. Переход стоимостью 5 не превращается в один приказ.
- Победа вычислена из захвата четырёх узлов и фактического развития (три протокола, слот tier 2); не устанавливается тестовой переменной.
- После победы тест собирает SaveGame из **наблюдаемых полей реальной партии** и использует `FAIHeroesCampaignRules::Validate`, `FAIHeroesSaveSystem::Save`, `FAIHeroesSaveSystem::Load` и проверку реконструкции победы. Слот — `AIHEROES_E2E_QA_<GUID>_A/B`, удаляется по завершении. Игровые пользовательские K/L методы не вызываются (у них действующие фиксированные слоты, которые нельзя перезаписывать в QA).

## Технические артефакты и проверка

- Исходник: `Source/AIHeroes/Private/Tests/AIHeroesFullCampaignJourneyTests.cpp`.
- Сборка: `Docs/BuildRuns/20261030T123448990Z/compiled.json`, 254 входных файла, DLL SHA-256 `a9940ed0c407b08adbaefd36cf2936f88d23c2871eaa594077267367a12bc37e`.
- Целевой Unreal run: `Docs/AutomationRuns/20261030T123824431Z`, **1/1 Success**, `build_verified=true`.
- Полный Unreal run: `Docs/AutomationRuns/20261030T123950754Z`, **283/283 Success**, failed=0, warnings=0, notRun=0, inProcess=0, `build_verified=true`; этот запуск выполнен защищённым параллельным QA-процессом на **той же проверенной DLL**.
- `Tools/verify_one_playable_campaign.py`: `STATIC_PASS_BUILD_MATCH`, 254 inputs, current source hashes match DLL.
- Начальные хеши пользовательских сохранений: `Saved/QA/FullCampaignJourney_20261030/user_saves_before.json`; после обоих прогонов четыре файла идентичны по имени, размеру и SHA-256. Префикс `AIHEROES_E2E_QA_` оставил 0 файлов.
- Собственный отчёт и файлы подпишет отдельная QA-квитанция; здесь никаких git commit/push/deploy не выполнялось.

## Чего проверка НЕ доказывает

- Не был выполнен десятидневный **ручной** проход мышью/клавиатурой с видимыми переходами по всем экранам, боевыми анимациями и отображением победы.
- Нативный E2E не использует клавиши K/L, а проверяет SaveSystem на безопасном временном namespace. Совместимость K/L с игровыми слотами требует отдельной тестовой или ручной проверки без перезаписи пользовательских файлов.
- Не проверены Android, упаковка Windows, настройка реальной LLM, ощущения игрока, меню/читабельность HUD на всех разрешениях и отдельные игровые режимы.
- Сборочная квитанция фиксирует `Source/`, `Config/`, `AIHeroes.uproject` и DLL; ассеты `Content/` сюда не входят. Старые GPU-кадры нельзя автоматически отнести к новой DLL.
- Путь к победе доказан **для одной конкретной последовательности игровых решений**. Это не доказательство сбалансированности или проходимости любого выбора.

## Следующий шаг

На новой DLL сделать игровой GPU-прогон минимум 1280×720 и 640×720 с кадрами после каждого ключевого действия (бой, пополнение, текущая цель, победная плашка), затем ручную приёмку K/L на изолированном тестовом профиле. Исключить пользовательские слоты из QA. Основание: Epic рекомендует Functional Testing/Gauntlet и отдельные screenshot/interaction проверки, а не подмену их успешными unit-тестами.

Источники: https://dev.epicgames.com/documentation/unreal-engine/automation-test-framework-in-unreal-engine ; https://dev.epicgames.com/documentation/unreal-engine/review-test-results-in-unreal-engine ; https://dev.epicgames.com/documentation/unreal-engine/saving-and-loading-your-game-in-unreal-engine

## ΔDΩΛ

- Δ: впервые проверена непрерывная игровая победа на настоящих правилах, а не на синтетической таблице целей.
- D: new native integration test → compiled 254 inputs → targeted 1/1 → full 283/283 → real victory day 10 → isolated save/read-back → zero user-save drift.
- Ω: 95% для проверенной одной автоматизированной партии и source-pinned нативной регрессии; ручной UX не проверен.
- Λ: новая исходная версия DLL, новая экономика/AI, невыполненная manual/UI приемка или возникший дефект.
