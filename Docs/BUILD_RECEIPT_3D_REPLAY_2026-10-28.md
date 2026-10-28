# AI HEROES — квитанция этапа 3D-боя и интеграции — 2026-10-28

Статус: PARTIAL / VERIFIED_BASELINE_PASS / CURRENT_LIVE_GRID_BUILD_BLOCKED.
Рабочее дерево: C:\github\AIHEROES (Windows DESKTOP-ITUAMNF).
Границы: локальный Windows прототип; не Android, не релиз, не real LLM.
Источник истины статуса: UnrealBuildTool, отдельные тестовые логи,
window-only screenshots и SHA-256 read-back.

## 1. Функциональный результат: проверенный первый 3D-повтор

Игрок выбирает стратегический узел и подтверждает бой. Детерминированный
FAIHeroesBattleEngine вычисляет HP и победителя, FAIHeroesBattleRecordRules
проверяет исходные армии, протокол и историю. По V/3D REPLAY V запускается
временная 3D-площадка, сферы героя, кубы врага, подсветка сторон,
последовательность ударов, индикаторы HP и численности на HUD.
Esc/Enter или SKIP 3D REPLAY возвращает стратегическую камеру.
Visual replay не изменяет состояние кампании. Реальных моделей 1B/3B/8B нет.

Новый typed receipt: Round, attacking side, stack indices, names, positive
damage, target HP before/after. VerifyReplay пересчитывает боевые изменения,
исход раундов, victory/defeat, полный summary и итоговые армии. Негативные
тесты защищают от подмены HP, урона, индексов, текста, раундов и победителя.

## 2. Успешные объективные проверки

A. Unreal Engine 5.8: AIHeroesEditor Win64 Development.
Версия baseline с 3D stage и высококонтрастным HUD собрана до внедрения
нового GridBattle v2, с результатом UBT Result: Succeeded.
Локальный скомпилированный gameplay и 3D replay QA запускаются.
Предупреждение MSVC: compiler 14.51 newer than preferred 14.50, без ошибки.

B. Команда автоматизации:
UnrealEditor-Cmd.exe AIHeroes.uproject -Unattended -NullRHI -NoSplash
-ExecCmds="Automation RunTests AIHeroes"
-TestExit="Automation Test Queue Empty"
-log=AIHeroesReplayFinalTests.log

Лог: Saved\Logs\AIHeroesReplayFinalTests.log.
Итог: 22 Test Completed / 22 Success / 0 Failure, процесс exit code 0.
Это снимок тестов ПЕРЕД незавершённым live GridBattle v2.
Особенно важные тесты: AIHeroes.Core.TypedBattleTimeline,
AIHeroes.Core.ReplayTamperRejected,
AIHeroes.Replay.DeterministicVerification,
AIHeroes.Replay.DiskRoundTrip и тесты маршрутов/сохранений.

C. Интерактивная оконная QA в реальной -game сессии:
Tools\qa_3d_replay.ps1 -TargetProcessId 57184.
Скрипт проверял PID, командную строку, размер viewport 1280x720,
затем посылал клики ТОЛЬКО в HWND выбранной игры; не перехватывал
глобальную мышь пользователя. Exit code 0:
GAME_PID_PINNED -> FOREST selected -> FOREST CAPTURED ->
3D REPLAY START 3 verified actions -> REPLAY 1/3 ->
SKIP -> 3D FINISH camera restored -> 3D_REPLAY_INTERACTIVE_QA_PASS.
Оба screenshots подтверждены непосредственным просмотром.
Отдельный поздний screenshot также показывает контрастные HUD-панели
HP обеих армий во время 3D-повтора.

## 3. Последняя разработка: текущий незавершённый интеграционный gate

Параллельно в общем репозитории начат GridBattle v2 с пошаговой live-сценой,
планировщиком маршрутов, версиями боевой истории и новыми тестами.
Редактирование в текущем чате не претендует на авторство всех этих файлов.
Последний Git HEAD, наблюдённый в процессе: 2e098e2, рабочее дерево
содержит параллельные изменения; никаких git commit/push здесь не выполнялось.

Заключительная сборка во время активной параллельной разработки выявила:
- C2084: двойная внутренняя функция TotalHP в unity-сборке Advisor и
  RoutePlanner. Мы безопасно переименовали локальный helper в
  RouteTotalHP в Source\AIHeroes\Private\AIHeroesRoutePlanner.cpp,
  без изменения формулы и с отдельной gitignored резервной копией.
- LNK2019: когда GridBattle.h и его тесты появились до его .cpp,
  требуемая реализация ещё не была слинкована; позже .cpp появился.
- UHT Error: Unable to find class AAIHeroesLiveBattleStage,
  потому что GameMode.h уже ссылался на незавершённый LiveStage-класс.
  В наблюдаемом состоянии на 22:05 файла класса ещё не было.
Состояние CURRENT_LIVE_GRID_BUILD = BLOCKED, а НЕ PASS.
Не удалять это объявление и не писать фиктивную реализацию, так как
соседний процесс активно создаёт новый live tactical module.

Уже существующий 3D replay v1 скомпилирован и визуально проверен.
Частные новые изменения в live GridBattle v2 требуют самостоятельных
сборки, тестов, ручного QA и release gate.

## 4. Контрольные файлы и хеши

Таблица представляет конкретные локальные байты на момент read-back,
не претендует на соответствие более поздним параллельным изменениям.

| Относительный путь от C:\github\AIHEROES | Байты | SHA256 |
|---|---:|---|
| Docs/3D_BATTLE_REPLAY_CONTRACT.md | 6322 | 3ca6f1ec251551e1ed478769297ed52d639aacd4d22f71f90e3a8fdc6cfff804 |
| Docs/AIHEROES_3D_REPLAY_INTERACTIVE_2026-10-28.png | 463960 | da8e142cd44655c1e89215141d788b75c60f7b16d01f4a9b87d768538b5206b5 |
| Docs/AIHEROES_3D_REPLAY_RETURN_2026-10-28.png | 385939 | 76edd740df3594e857d0fc607f5ccca03fbc7afa5206c7f23eb20f4b875e5e62 |
| Docs/AIHEROES_3D_REPLAY_LIT_2026-10-28.png | 449819 | 13722b6f364a23b19053fc6fc1890199708fcb76ea0634c6cb90502a9c7ebb8a |
| Source/AIHeroes/Public/AIHeroesBattleEngine.h | 1732 | c54c56b4e818af5cca535cfa2368be42bd812ab6118ffc02558a6756a7d59d65 |
| Source/AIHeroes/Private/AIHeroesBattleEngine.cpp | 9642 | 8136bb815695f2d510cd121c30465831f3bb3700601aadc36b6030951fc0178f |
| Source/AIHeroes/Public/AIHeroesBattleStage.h | 2494 | 3d0a3566c41bb2c678b4066a50af7aab1b56196c3c70a0e4509118c781ffd96c |
| Source/AIHeroes/Private/AIHeroesBattleStage.cpp | 10494 | 0ef5a31bc166f8a12d5a608e90fcdb632c19a98da00a8c1bbfc6bf1309f1557f |
| Source/AIHeroes/Private/Tests/AIHeroesBattleTimelineTests.cpp | 4954 | a73a8ff4c6840d344d0f7a63eb383972ed775433c0d75294690222ad8b41f106 |
| Tools/qa_3d_replay.ps1 | 4058 | 6ea46343d16e9f7726b7d16e1630f00889f0cf0e5418c404c1ec8b373f6bd151 |

Модуль AIHeroesBattleReplayGameMode.cpp не зафиксирован здесь хешем
ввиду изменения этого файла параллельным процессом после validated build.

## 5. Обязательные следующие ворота

1. Дождаться фактического завершения класса AAIHeroesLiveBattleStage
   в коде; не принимать объявление за реализацию.
2. Пересобрать весь AIHeroesEditor Win64 Development без editor DLL lock.
3. Выполнить весь Automation RunTests AIHeroes с отдельным логом.
4. Провести в окне -game live-grid сценарий: действия шаг за шагом,
   pause/speed/fast finish, итог, сохранение и повтор после загрузки.
5. Сверить итоговые армии, ресурсы и карту при неудаче/пропуске,
   Android SDK и производительность проверять независимо.
6. Лишь после проверки отметить разработку как integrated / playable.
   Текущий статус live-grid: UNVERIFIED / BUILD_BLOCKED.

## 6. Non-claims

НЕТ Android APK, реальной LLM-модели, скачанных 3B/8B весов,
полной анимационной сцены с skeletal characters, production release,
автоматического GitHub push, merge или деплоя.
Сохранение игровой кампании НЕ тождественно долговременной памяти ИИ.

PASS: 3D replay v1 baseline, typed replay validation, 22 baseline tests,
window-targeted replay + skip QA, SHA screenshots.
PARTIAL/BLOCKED: новая live-grid интеграция из параллельного рабочего дерева.

∆DΩΛ:
∆: проверяемое тактическое воспроизведение в настоящей 3D-сцене.
D: source diff -> UE build -> 22 automation tests -> 3D gameplay -> PNG+hash.
Ω: 95% для перечисленных baseline тестов, низкая для будущего GridBattle v2.
Λ: повторный билд после появления и подключения LiveBattleStage, затем live QA.
