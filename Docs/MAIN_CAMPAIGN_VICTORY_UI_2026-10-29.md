# AIHEROES — читаемое завершение одной основной кампании

Дата: 29 октября 2026. Статус: **SOURCE_IMPLEMENTED / PORTABLE_CXX_METHODS_PASS / UNREAL_BUILD_BLOCKED_MEMORY / GAMEPLAY_QA_PENDING**.

## Принятое направление

Единственная стартовая игра — действующий основной `AIHeroesGameMode`. В первой итерации не заменять его экспериментальной `ZoneExpedition`, не мигрировать несовместимые SaveGame, не вводить ещё один GameMode или дублирующий игровой контур.

Петля должна быть понятной без технических инструкций:
**цель → база/найм → разведка/маршрут → подтверждение игроком → автономное сражение героя → потери/награда → развитие → подтверждённая победа → сохранение/продолжение.**

Система `FAIHeroesCampaignObjectives::Evaluate` уже определяет победу по четырём условиям: FOREST+ORE, OUTPOST+RUINS, ProtocolTier=3, ModelSlotTier=2. Она не является реальным нейросетевым инференсом.

## Новый пользовательский экран победы

Добавлены:
- `Source/AIHeroes/Public/AIHeroesVictoryPresentation.h`
- `Source/AIHeroes/Private/AIHeroesVictoryPresentation.cpp`
- `Source/AIHeroes/Private/AIHeroesVictoryHUD.cpp`
- `Source/AIHeroes/Private/Tests/AIHeroesVictoryPresentationTests.cpp`

Изменены только UI-связки `Source/AIHeroes/Private/AIHeroesHUD.cpp` и `Source/AIHeroes/Public/AIHeroesHUD.h`.

На широком экране после выполнения всех четырёх целей рядом с полосой задач появляется небольшая тёмно-сине-золотая карточка:

**КАМПАНИЯ ЗАВЕРШЕНА**

*Все 4 цели выполнены. Рубежи под защитой Искры.*

*Нажмите: итоги [O] · Сохранить [K] · Играть дальше.*

Блок не модальный. Нажатие внутри него вызывает **существующее действие 202 — открыть read-only досье целей**, вместо пустого поглощения клика. Оно не делает ход, не начисляет ресурсы, не изменяет SaveGame и не даёт ИИ права на стратегический приказ. Поскольку нажатие перехватывает HUD, оно не попадает на карту под ним. При открытом меню или досье карточка скрывается; на узких экранах остаётся компактная карточка цели.

В отображение не верят по одному boolean `bVictory`: дополнительно проверяется количество целей и их действительная завершённость. Карточка использует только `GetCampaignProgress`, извлечённый из реального игрового состояния.

## Автотесты (НАПИСАНЫ, ЕЩЁ НЕ ЗАПУСКАЛИСЬ)

Новые группы:
- `AIHeroes.Playability.VictoryOverlay.OnlyFromCompleteObjectives` — новые, подменённые, неполные и завершённые состояния.
- `AIHeroes.Playability.VictoryOverlay.MapBoundsAndInputs` — геометрия на 1150×640, 1280×720, 1600×900, отсутствие перекрытий миссии, инспектора, нижних кнопок и мини-карты, узкие экраны и открытое меню.
- `AIHeroes.Playability.VictoryOverlay.ReadOnlyGoalsHitTest` — клик в карточку вызывает только действие 202; вне карточки, при открытом меню, узком экране или неполной победе — ни одного действия.

Эти тесты не подменяют настоящее непрерывное прохождение от начала до победы. Чистый portable MSVC тест фактических тел `FAIHeroesExplorerLayout::Frame` и `FAIHeroesVictoryPresentation::{IsAuthoritativeVictory,Banner,HitTest}` прошёл **36/36 проверок**. Авторский скрипт: `Tools/verify_victory_overlay_portable.py`; результат: `Saved/QA/VictoryPortable_20261029T173358436284Z/receipt.json`. Он компилирует функции на узких mock-типах, **не** проверяет UE API, DLL, GPU или пользовательскую игру.

## Проверка и блокеры

Сделаны сохранённые точные копии до правок:
`Saved/QA/VictoryPresentation_20261029/Source_AIHeroes_Private_AIHeroesHUD.cpp.before`
и `Saved/QA/VictoryPresentation_20261029/Source_AIHeroes_Public_AIHeroesHUD.h.before`.
Проверка `git diff --check` по двум затронутым tracked-файлам не нашла ошибок пробелов.

При работе в **открытом VS Code** обнаружен и корректно исправлен UX-пробел: карточка победы поглощала нажатие, но не открывала итоги. Предварительное предположение о «смещении полосы заданий на 12 px» после чтения `AIHeroesExplorerLayout::Frame` **не подтвердилось**: в текущем layout координаты `Mission.Y` и `Map.Y` совпадают. Добавленный расчёт через `FMath::Max` лишь защищает от будущего расхождения, не считается исправлением текущего бага.

Две полные попытки сборки `AIHeroesEditor Win64 Development` выполнялись без закрытия VS Code. Первая (лог `Docs/BuildRuns/20261029T172754393Z/build.log`) завершилась `C3859/C1076`, Windows 1455, на компиляции соседнего `AIHeroesBasePanelCopyTests.cpp`; вторая (лог `Docs/BuildRuns/20261029T172858033Z/build.log`) — `C1060` при компиляции нового `AIHeroesBattleAftermathTests.cpp`. Оба отказа связаны с нехваткой памяти в наблюдаемой среде; это **не** свидетельство прохождения или провала новых функций внутри Unreal. Защита сборочного скрипта удалила `Docs/AIHEROES_BUILD_LATEST.json`, чтобы не оставить устаревшую версию за текущую; прежний манифест предварительно скопирован в `Saved/QA/OnePlayableCampaignBuild_20261029/PREVIOUS_BUILD_LATEST.json`. Настройки файла подкачки, пользовательские программы и сохранения не изменялись. Полную Unreal Automation и GPU-QA не запускали.

Существующие параллельные правки `AIHeroesCampaignGuide`, `AIHeroesExplorerHUD`, `AIHeroesPlayableJourneyTests` и другие файлы не перезаписывались. Не выполнялись Git commit/push/reset, изменения базы данных, форматов сохранения или поставки.

## Чёткое определение одной играбельной партии

Для PASS требуются одновременно:

1. Свежая компиляция полного текущего `Source/` через `Tools/build_aiheroes_editor.ps1`, чтение `compiled.json`.
2. Полный `Tools/run_aiheroes_automation.ps1` на том же SHA-256-снимке; `failed=0`, `notRun=0`, `warnings=0`, включая новые `VictoryOverlay`.
3. Человек может начать новую основную кампанию, получить подсказку, выбрать маршрут и решить, атаковать ли охраняемую точку. ИИ проводит бой; результат показывает потери и награды.
4. Реально доступны все четыре условия победы без команд разработчика. Нет автоматической покупки, автосохранения или смены GameMode.
5. После победы игрок видит карточку, **клик по ней** и кнопка O открывают итоги без приказа по карте, K сохраняет, а повторная загрузка восстанавливает победу.
6. Настоящие GPU-кадры **одной сборки**: старт, база, карта, совет, бой, итог, победа, восстановление. Отдельный просмотр на PC 1280×720 и компактном размере без перекрытия текста и действий.
7. Сохранения пользователя и побочные процессы не изменены QA-прогоном. Тесты используют собственные изолированные слоты и собственный QA-процесс.

До этих проверок статус всей игры — НЕ `verified playable`, даже если отдельные автоматические тесты успешны.

## Внешние технические основания

- Unreal Gameplay Framework: https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-framework-in-unreal-engine
- AHUD Canvas API: https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/AHUD
- Unreal Automation Test Framework: https://dev.epicgames.com/documentation/unreal-engine/automation-test-framework-in-unreal-engine
- Unreal Save/Load: https://dev.epicgames.com/documentation/unreal-engine/saving-and-loading-your-game-in-unreal-engine
- Microsoft XAG 101, читаемость текста: https://learn.microsoft.com/en-us/gaming/accessibility/xbox-accessibility-guidelines/101

∆DΩΛ: ∆ — карточка настоящей победы стала кликабельной точкой входа в существующие итоги; D — VS Code diff, 36/36 portable MSVC, резервные копии, SHA-256, две прозрачные ошибки полной Unreal сборки; Ω — 94% для контрактов функций в изоляции, Unreal-готовность неизвестна; Λ — стабильная память → source-pinned Unreal сборка → полная автоматизация → GPU-кадры → непрерывный игровой проход.
