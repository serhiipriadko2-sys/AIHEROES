# AIHEROES — живой бой остаётся понятным без 3D-арены

Дата: 2026-10-29.
Статус: SOURCE_IMPLEMENTED / STATIC_QA_PASS / NATIVE_BUILD_NOT_VERIFIED.
Режим: основная пятиузловая кампания AIHeroesGameMode, тактика Grid V2.

## Откуда возникла задача

В аудите Docs/PROJECT_COHESION_AUDIT_2026-10-29.md описан разрыв F2:
после ошибки показа анимации LiveBattleGameMode выполняет
ClearLiveStage(false) и продолжает прежнюю проверяемую тактическую сессию.
Однако HUD без Stage раньше показывал «Подготавливаем поле боя…»,
«Раунд 0» и пустые списки отрядов. Игра сохраняла расчёт боя, но игрок
терял информацию о происходящем.

## Что теперь получает игрок

- Существует 3D-арена: прежние ростер, портреты, анимации, подсказки камеры
  и отображение боя сохранены.
- 3D-арена отсутствует, сражение идёт: HUD сообщает о текстовом режиме,
  количестве фактически выполненных действий и текущем составе отрядов
  обеих сторон, который уже известен активному бою.
- Бой приостановлен: состояние прямо помечено как пауза, без обещания
  происходящих в это время действий.
- Тактический движок завершил сражение и применил результат: доступен
  возврат на карту, а существующий BattleAftermath показывает реальные
  потери собственной армии. Отдельные награды не выдумываются.
- Если строки отрядов пусты, выводится «Сведения об отрядах пока недоступны»,
  а не фиктивная численность.

Для текста используется уже существующий GameMode:
GetLiveBattleActionCount(), GetLiveArmyStatus(true/false). В HUD не
создаётся другой расчёт боя, новая награда, новое действие ИИ или SaveGame.
Число действий не выдаётся за число раундов; на нормальной 3D-сцене
по-прежнему показывается собственно представленный раунд.

## Изменения файлов

- Source/AIHeroes/Public/AIHeroesBattleFallbackView.h
- Source/AIHeroes/Private/AIHeroesBattleFallbackView.cpp
- Source/AIHeroes/Private/Tests/AIHeroesBattleFallbackViewTests.cpp
- Source/AIHeroes/Private/AIHeroesBattleHUD.cpp

FallbackView — отдельное read-only представление. Оно разделяет строки
стеков по существующему разделителю GameMode (максимум два стека с каждой
стороны), переводит известные обозначения «units / HP / INIT» для игрока
и готовит копию, но не изменяет GameMode. Настоящий HUD рисует две
карточки в уже существующей геометрии BattleArt::RosterRect.

## Новые тесты Unreal

1. AIHeroes.Playability.LiveBattleFallback.RunningKeepsKnownArmies
2. AIHeroes.Playability.LiveBattleFallback.PausedDoesNotClaimProgress
3. AIHeroes.Playability.LiveBattleFallback.ResultOffersReturn
4. AIHeroes.Playability.LiveBattleFallback.ArenaAvailableIsSilent
5. AIHeroes.Playability.LiveBattleFallback.EmptyAndNegativeInputSafe

Они написаны, но до свежей компиляции не объявляются выполненными.
Статический source gate: 16/16 PASS, две проверки на искусственно удалённый
guard PASS; git diff --check для HUD PASS. Проверено совпадение SHA-256
перед правкой и копии для отката, контрольные хеши файлов — в отдельной
квитанции. Два ранних билда в Docs/BuildRuns/20261029T172754393Z и
20261029T172858033Z завершились ошибками памяти C1076/C1060;
они не подтверждают эту новую версию.

## Обязательная нативная проверка

1. Согласовать стабильный снимок Source/ и Config/, освободить ресурсы
   штатным способом без завершения чужих процессов.
2. Запустить Tools/build_aiheroes_editor.ps1 и проверить fresh compiled.json
   с исходными хешами и соответствием DLL.
3. Запустить Tools/run_aiheroes_automation.ps1 для фильтра
   AIHeroes.Playability.LiveBattleFallback, затем весь набор AIHeroes
   с тем же compiled snapshot, изучив экспортированный JSON.
4. В контролируемом тестовом мире разрушить только disposable
   LiveBattleStage (не настоящие пользовательские объекты),
   проверить показ текстового боя, паузу, ускорение и возврат на карту.
5. Снять реальные игровые кадры на 1280x720 и 640x720 после той же
   сборки. Проверить читаемость двух сторон, паузы, действия и кнопок.
6. Проверить неизменность исхода, журнала, начислений и пользовательских
   сохранений относительно обычной 3D-сессии.

До этих действий статус: SOURCE_IMPLEMENTED_STATIC_QA_PASS,
но не compiled, verified-live или playable release.

## Неприкосновенные границы и откат

Не менялись GridBattle, GameMode, LiveSession, награды, экономики, маршруты,
SaveGame, канон, Git и пользовательские сохранения. Для точного отката
исходного участка HUD сохранена копия:
Saved/QA/PlayableBattleFallback_20261029/AIHeroesBattleHUD.cpp.before
SHA-256: 782b56f938d1bf70d682f19c362d90f3e111806b0fc3fd18eb314327c05dfc41

При откате сначала сверить хеши: удалить только новые три файла и
вернуть только наши изменения HUD, не применять git reset/clean и не
переписывать чужие незакоммиченные правки.

## Внешние инженерные ориентиры

- Epic UE 5.8 User Interfaces and HUDs:
  https://dev.epicgames.com/documentation/unreal-engine/user-interfaces-and-huds-in-unreal-engine
- Epic Automation Test Framework:
  https://dev.epicgames.com/documentation/en-us/unreal-engine/automation-test-framework-in-unreal-engine
- Epic Run Automation Tests:
  https://dev.epicgames.com/documentation/unreal-engine/run-automation-tests-in-unreal-engine
- VS Code CLI для наблюдения изменений:
  https://code.visualstudio.com/docs/configure/command-line

ΔDΩΛ: Δ — резервный экран боя отображает доступное состояние без 3D-сцены;
D — анализ текущих исходников + минимальная HUD-интеграция + пять новых
тестов + статический QA; Ω — уверенность в исходниках не выше 95%,
в игровом runtime неизвестна; Λ — нативная компиляция и проверка реального
fallback-сценария на едином снимке кода.
