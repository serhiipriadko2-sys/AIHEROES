# AIHEROES — GridBattle V2: исторический 3D-повтор

**Дата:** 2026-10-28  
**Проверенная поверхность:** Windows `C:\github\AIHEROES`, Unreal Engine 5.8  
**Статус:** `EDITOR_BUILD_PASS | AUTOMATION_77_77_PASS | REAL_WINDOW_REPLAY_PASS | SAVEGAME_IMMUTABILITY_PASS | ANDROID_NOT_RUN | ACTUAL_LLM_NOT_IMPLEMENTED`

## Задача и граница авторизации

Новый живой GridBattle V2 создавал в кампании проверяемый журнал сражения, но просмотр через клавишу V поддерживал только наследуемый V1. Этот этап добавляет просмотр **уже состоявшегося** V2-боя, не повторяет применение наград, потерь или захвата. История боёв не является разрешением отдавать новые игровые команды.

Источники: SoT30 v5.5.7 (локальный Knowledge 30/30, `support/PROJECT_INSTRUCTIONS_SOT30.md`), текущие файлы GameMode/GridBattle/RecordRules, скомпилированный бинарник и свежие тестовые журналы.

## Реализовано

1. `Source/AIHeroes/Public/AIHeroesGridReplay.h` и `Private/AIHeroesGridReplay.cpp`: `FAIHeroesGridReplayRules::Build` строго принимает только проверенный V2-record, повторно вычисляет боевые действия `Step` в памяти и сравнивает **каждую** типизированную запись с авторитетным текстовым журналом, включая конечный результат. При ошибке весь результат пуст; V1 не переинтерпретируется как V2.
2. `Private/AIHeroesBattleReplayGameMode.cpp`: V1 оставлен без изменения игровой модели; V2 использует отдельную копию `AAIHeroesLiveBattleStage`, последовательную презентацию доказанных событий, отдельную камеру, возврат на карту, команды `V / Space / T / Esc / B`. Replay **не вызывает** `StartLiveBattle` или `CommitLiveBattle`; состояние кампании сверяется по отпечатку на выходе.
3. `Private/AIHeroesLiveBattleStage.cpp`: безопасное read-only отображение текущей численности и HP обеих армий на HUD; полностью переиспользован имеющийся 3D рендер.
4. `Private/AIHeroesPlayerController.cpp`: во время V2-повтора Space переключает паузу, T скорость, B/Esc пропуск. Во время настоящего боя эти клавиши сохранили прежнюю семантику.
5. `Private/Tests/AIHeroesGridReplayTests.cpp`: три Unreal Automation теста — точность действий, fail-closed защита от подмены с совместимостью V1 и изолированная 3D-стадия.
6. `Tools/qa_grid_v2_replay.ps1`: запуск **только для точного PID изолированного** окна `-game -log=AIHeroesGridReplayQA.log`. Скрипт не делает глобальных нажатий, не сохраняет кампанию и сравнивает SHA-256 всей папки SaveGames до/после.

## Верификация

- `python Scripts/verify_sot30.py --directory .iskra/sot30`: PASS, 30/30 Knowledge, 32/32 SHA256SUMS; manifest SHA256 `0decd118a6300324b757d2587ec315a569aaf5800299d2b33bf046feaf56aeff`.
- Editor: `dotnet .../UnrealBuildTool.dll AIHeroesEditor Win64 Development -Project=C:\github\AIHEROES\AIHeroes.uproject -WaitMutex -NoHotReload -NoUBA -MaxParallelActions=1` → `Succeeded`, exit 0.
- Первый автоматический прогон получил crash в **параллельном тесте целей**: `Tests/AIHeroesCampaignObjectivesTests.cpp:122`, опасный `TArray.Add(TArray[Index])`. Создан резервный файл `Saved/IskraBackups/CampaignObjectivesTArrayAlias_20261028_2305.cpp` (8 319 байт, SHA-256 `ff3e8dfc8273e8befe3b18e2fb8ae27784c8c378c8cc229ddc2d33d888583284`). Тест исправлен через независимую копию элемента, смысл проверки дубликата сохранён; постфикс Editor build PASS.
- Следующий прогон: 76/77 Success; наш новый V1-тест по ошибке использовал две вражеские группы, что запрещено именно V1-форматом. Фикстура исправлена на одну группу без ослабления RecordRules; повторный Editor build PASS.
- **Финальный** свежий Unreal Automation Framework JSON: `Docs/AutomationRuns/20261028T200750062Z/index.json` → `77 cases, succeeded=77, failed=0, notRun=0, inProcess=0`. Среди них `AIHeroes.Replay.GridV2VerifiedTypedParity`, `GridV2RejectTamperAndLegacy`, `GridV2ReadOnly3DStage`.
- Настоящее изолированное GUI-окно Unreal: целевой PID 40892, 1280×720. `Saved/Logs/AIHeroesGridReplayQA.log` фиксирует: выбор FOREST → игрок подтвердил нападение → живой боевой `COMMIT` **ровно один раз**, `actions=9` → возврат → V → `[AI HEROES GRID REPLAY] START ... verified_actions=9 campaign_commit=0` → 1/9 → PAUSED → SPEED 2x → RESUMED → SKIP → `FINISH camera_restored=1 campaign_unchanged=1`. Итог скрипта `GRID_REPLAY_REAL_WINDOW_PASS`, `GRID_REPLAY_SAVES_UNCHANGED_PASS`, exit 0.
- GUI QA создал три PNG **1296×759** (включая рамку окна). Статическая проверка по 36 точкам показала 12–16 разных пиксельных цветов: изображения не пустые, но это не заменяет ручную визуальную приёмку дизайна. Отдельное QA-окно штатно закрыто; пользовательские файлы сохранений не менялись.
- `git diff --check`: exit 0; замечания Git о будущей нормализации LF→CRLF не являются сборочным FAIL.

## Контрольные суммы артефактов

| Путь от `C:\github\AIHEROES` | Байты | SHA-256 |
|---|---:|---|
| `Source/AIHeroes/Public/AIHeroesGridReplay.h` | 1120 | `71d25acab416e6abceb93ab933c5ab4ee7832733e4bb11750aaff4d0264feb06` |
| `Source/AIHeroes/Private/AIHeroesGridReplay.cpp` | 2879 | `7b53ff39ef3c9143fcfec1ee6dde3ddd9ef3fb879474bff837367a05a57c3de4` |
| `Source/AIHeroes/Private/Tests/AIHeroesGridReplayTests.cpp` | 8078 | `2b97da5ede477e2bd3ffc79ab91b858ee60c9e685269c4b869228bcab1c086b0` |
| `Source/AIHeroes/Private/AIHeroesBattleReplayGameMode.cpp` | 11895 | `b55fe385821aa6505ab584d650b8b28dad160222259be63e184bd4ff7e58dbbb` |
| `Source/AIHeroes/Private/AIHeroesLiveBattleStage.cpp` | 23239 | `39459a8df2458ed29aebc3d0638ad3fb71cfa4d661881cb7bb0a8e619bcf783b` |
| `Source/AIHeroes/Public/AIHeroesLiveBattleStage.h` | 4341 | `7a69391e7287f293e18e0d766600c6e179510e8ef1a6aabd7c1e5d8eacb1e309` |
| `Source/AIHeroes/Private/Tests/AIHeroesCampaignObjectivesTests.cpp` | 8531 | `8ad508a5c3a71131839112f273e93be78cc61a475e7c18bb9f94b71c9c2e80a4` |
| `Tools/qa_grid_v2_replay.ps1` | 6276 | `fb22811c0f413f51b8c360c7ea4ec7ee15de93e9ec74c4e05d2b2796a324b963` |
| `Docs/AutomationRuns/20261028T200750062Z/index.json` | 30482 | `1ca37ab38b338d582e1013fdb29d24c7edd3ed873b8b54bb4a8831775c38c15b` |
| `Saved/Logs/AIHeroesGridReplayQA.log` | 196290 | `aff9ec6345be06497d6dd748b43ff28942feaef7b18e393a50443e1f80fc4a1b` |
| `Saved/QA/GridV2Replay/GRID_REPLAY_V2_START.png` | 417784 | `11eacec48a04471149209c532775454d414019770112729825e8abd5258969b5` |
| `Saved/QA/GridV2Replay/GRID_REPLAY_V2_PAUSED.png` | 360219 | `ef01280bbc2f3aa8913682dcf317c398462b4bb18589c608662b39014560df3e` |
| `Saved/QA/GridV2Replay/GRID_REPLAY_V2_RETURN.png` | 356084 | `ae0ee2ed875fa6c6d634e828ce593b3c10d5585ce9c5186ba4b495deb2c53508` |
| `Binaries/Win64/UnrealEditor-AIHeroes.dll` | 1094656 | `3b26336a6a93c585924cfa8a6519ac476d0ed24c42971d151146bd5aa1d55082` |
| `Docs/LIVE_GRID_V2.md` | 4704 | `81480e6d2cf7f8adadf6b5606a2fd518e4bd646127a8611ef3fe66e6ef2afe28` |

## Что НЕ сделано

- Не делалось обновление Android APK и не проверялось устройство Android.
- Реальных локальных моделей 1B/3B/8B нет: ИИ использует детерминированные игровые правила.
- GUI-прохождение проверено автоматическим управлением конкретным QA-окном; это не доказательство полного UX на телефоне и не проверка всех пикселей вручную.
- Параллельно другие разработчики вносят базу, цели и AI Proposal Gate. Я не сбрасывала их файлы, не коммитила, не выполняла push, merge или deploy. До выпуска — заморозить общее рабочее дерево и снова выполнить полный gate.

**Фактический статус:** `LOCAL_WINDOWS_BUILD_PASS | AUTOMATION_77_77_PASS | V2_REPLAY_GUI_SMOKE_PASS | SAVEFILES_UNCHANGED | RELEASE_NOT_VERIFIED`.

**∆DΩΛ:** ∆ — проверяемое 3D-воспроизведение V2 с паузой, скоростью, пропуском и возвратом; D — исходники, тесты, Windows DLL, JSON-отчёт, окно игры и три изображения; Ω — 95% для наблюдённых Windows-gate, ниже для Android/реального LLM; Λ — новое изменение источников/коммит или Android-перенос требует свежей компиляции и регрессий.
