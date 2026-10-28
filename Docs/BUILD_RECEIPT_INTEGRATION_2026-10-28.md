# AIHEROES — интеграция игрового цикла и раздельной эволюции ИИ

Дата: 2026-10-28
Поверхность: авторизованный Windows PC, C:\github\AIHEROES
Статус: LOCAL_BUILD_PASS / AUTOMATION_PASS / INTERACTIVE_QA_PASS / ANDROID_BLOCKED
Граница: локально собранный ПК-прототип; НЕ готовый релиз, НЕ Android APK, НЕ запущенная LLM.

## Изменение и функциональность

- Активный GameMode: Source/AIHeroes/Private/AIHeroesGameMode.cpp.
- Стратегический цикл: выбрать 3D-узел -> запросить совет -> подтвердить ход -> 
  автономный бой отрядов под управлением правил героя -> добыча -> следующий день.
- Бюджет перемещения = 4 очка/день, стоимость расстояния проверяется до боя.
- Точечный захват и маркер позиции героя на реальной 3D-карте.
- HUD: семь больших кнопок действий, отдельные SAVE/LOAD из текущей реализации.
- Два НЕЗАВИСИМЫХ параметра: ProtocolTier (SEED/ANALYST/...) и
  ModelSlotTier (1B/3B/8B игрового слота без подключённых весов).
- DataFragments: 2 за обычную победу, 4 за руины. Открытие 3B-слота
  требует 60 GOLD и 2 DATA, не меняет уровень протокола. Протокол отдельно
  покупается из древесины/руды/золота и изменяет игровые решения.
- Game SaveGame записывает оба параметра; ограниченная валидация чтения
  и disk round-trip проверены тестом, F5/F9 ручной игровой сценарий
  отдельно не проверен, чтобы не перезаписать пользовательский fixed save slot.
- AIH* альтернативный прототип сохранён; активен AIHeroesGameMode.

## Реальные команды и проверки

1. UnrealBuildTool: AIHeroesEditor Win64 Development, -Project=C:\github\AIHEROES\AIHeroes.uproject,
   Result: Succeeded, exit code 0. Команду запускали после штатного закрытия
   только нашего игрового QA-процесса, который удерживал DLL.
2. UnrealEditor-Cmd: -Unattended -NullRHI
   -ExecCmds="Automation RunTests AIHeroes"
   -TestExit="Automation Test Queue Empty".
   Результат из C:\github\AIHEROES\Saved\Logs\AIHeroes_2.log:
   11 tests performed; 11 Test Completed Result={Success}; 0 non-Success.
   Проверены Advisor.GlobalStrategicChoice, Advisor.ProgressionAndForecast,
   Campaign.DiskRoundTrip, Campaign.Validation, Core.AtomicResourceSpend,
   Core.DeterministicBattle, Core.ModelSlotPurchase, Core.NoArmyCannotWin,
   Core.ProtocolChangesTactics, Core.TravelBudget, Simulation.CombatPolicy.
3. C:\github\AIHEROES\Tools\qa_playthrough_game.ps1 посылает события
   мыши только проверенному -game окну Unreal, не перехватывает курсор
   пользователя и проверяет фактические события по игровому логу.
   Result: INTERACTIVE_QA_PASS, exit code 0.
4. В игровом журнале подтверждены: FOREST selected -> AI advisory ->
   FOREST CAPTURED, AI autonomous VICTORY (1 round, 11 surviving units) ->
   Day 2 income -> MODEL SLOT 3B unlocked -> Unlocked THREAT ANALYSIS ->
   ORE selected -> advice/forecast. Trace: NPC Wolf attacked Militia,
   hero Scout and Militia independently attacked Wolf.
5. Визуальный контроль ДВУХ скриншотов:
   A: SEED | MODEL SLOT 3B (NO CHECKPOINT);
   B: ANALYST | MODEL SLOT 3B (NO CHECKPOINT).
   Это показывает независимость игровой модели и протокола.

## Воспроизводимые артефакты

| Путь относительно C:\github\AIHEROES | Байты | SHA-256 |
|---|---:|---|
| Binaries/Win64/UnrealEditor-AIHeroes.dll | 440320 | fdf1228a818b84096a93e869c60d009d9ef69a93ca2931c5b20083610b917dfa |
| Docs/AIHEROES_SLOT_ONLY_2026-10-28.png | 379030 | b46b5e7e957ed7db53153d69c09cb8f30fc9e5fc54d38b46986356909a2648d3 |
| Docs/AIHEROES_SLOT_PLUS_PROTOCOL_2026-10-28.png | 378924 | 637202091493750a59e1dad3ce28d8ff7b305bbdb8c426ea30e7d0af9117fdb1 |
| Source/AIHeroes/Private/AIHeroesGameMode.cpp | 20577 | 637f1b95bfbb9138e7929f8cf6ab001923373162e2d426c0b484081a6299a4fe |
| Source/AIHeroes/Private/AIHeroesHUD.cpp | 7284 | c27a7144cf759dc614c44a94d9d24a26974b8f7c8ca7876686c7ff88d8085d84 |
| Source/AIHeroes/Private/AIHeroesPlayerController.cpp | 4548 | bfd8716a7f82f86564bca134024f0127bc8f1400e35f8b64bd6c8a9c5ad60d41 |
| Source/AIHeroes/Public/AIHeroesTurnRules.h | 1375 | 7a4b28cacc1bbbbbf30019ac2ef8d3a73de494c37b90cbc3824779bed6637178 |
| Tools/qa_playthrough_game.ps1 | 4218 | 3b60080b0a3e9bd19d377008c00e47e16339b10905777a8120c99a0dbfb3316b |
| README.md | 3137 | 815ba9b44db9c81da30d12d8a506566cc86ebaa8bc4ea6944ac501c7eac1e01f |
| .vscode/tasks.json | 2468 | eed29890d62f1ac22f9ed6f8b62d37836f4f36db2fba1e96d155519acf2913ec |

Проверка копий: обе картинки скопированы из Saved/Screenshots в Docs,
исходные и конечные SHA256 совпадают. JSON VSCode tasks: синтаксис валиден, 4 задачи.

## Неверифицированные границы и долги

- НЕТ реального 1B/3B/8B весового чекпойнта, обучения модели, микросервиса
  вывода ответов или полного SoT30 на персонаже. Надпись MODEL SLOT — слот,
  NOT actual inference.
- Нет 3D анимации поля боя: бой пока симулируется по инициативе и журналу.
- Android SDK всё ещё INVALID r27c, сборки APK и теста на телефоне НЕТ.
- Локальная игра сохраняется через Unreal SaveGame; это не память ИИ и
  не пользовательский доступ к файлам.
- В репозитории другая разработка менялась параллельно. При read-back
  HEAD=fc08d52, рабочее дерево содержало 31 изменённый/новый файл.
  Эта операция не выполняла Git commit, push, merge или deploy.
- Нельзя считать игру релизной по этим ПК-тестам; отсутствуют Android QA,
  сохранения со старых версий, баланс, глубокий тест автономного боя и внешний LLM.
- Backups отдельных редактированных файлов внутри git-ignored Saved/
  сверены SHA256. Не удалять без ревью.

## Следующий обратимый шаг

Открыть локальную игру из VSCode:
Terminal > Run Task > AIHEROES: Launch playable Windows game.
Проверить F5/F9 на отдельном безопасном сохранении после проверки существующего
слота. Затем исследовать реальную модель 1B только через typed bounded adapter,
не позволяя LLM прямо менять движок или файлы компьютера.

∆: протокол и модельный слот разделены в работающем игровом состоянии.
D: исходники -> UBT PASS -> 11 automation PASS -> UI playback PASS -> png/hashes.
Ω: 95% для локальной сборки и перечисленных тестов; Android и реальный LLM UNKNOWN.
Λ: пересмотр после первого Android build и проверки реальных весов/скорости/памяти.
