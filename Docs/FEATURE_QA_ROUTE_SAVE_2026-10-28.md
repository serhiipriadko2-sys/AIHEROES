# AIHEROES — надёжные сохранения и стратегия ИИ, локальная квитанция

**Дата:** 2026-10-28  
**Рабочая папка:** `C:\github\AIHEROES`  
**Статус:** **PC_COMPILE_PASS / AUTOMATION_22_OF_22_PASS / VISUAL_NEW_FEATURE_QA_NOT_RUN_BY_THIS_SESSION**.  
**Граница:** только локальный Windows-прототип. Это не Android-релиз и не подключённая LLM.

## Реализовано

### 1. Резервируемые сохранения кампаний
`FAIHeroesSaveSystem` из `Source/AIHeroes/Private/AIHeroesSaveSystem.cpp` создает две чередующиеся сохранённые версии A и B в фиксированном пространстве имён Unreal SaveGame. Новая версия пишется в альтернативный слот и проверяется повторным чтением; старый корректный слот не перезаписывается. Система игнорирует неверную схему, некорректные ресурсы, повреждённые данные и последовательности, возвращает предыдущую корректную копию и сообщает о восстановлении. Старый `AIHEROES_Campaign_v1` читается в качестве резервного источника, но не перезаписывается при миграции. Для `UAIHeroesCampaignSave` введено `SaveSequence` с начальным нулём для старых файлов. Запись самой игровой кампании по-прежнему разрешена только игроку.

Соседняя параллельная реализация добавила выбор трёх пользовательских кампаний в `AIHeroesGameMode`; каждая использует свою пару A/B. Это не три модели ИИ и не Iskra Memory. Для корректного ввода/вывода всегда используется встроенный Unreal SaveGame, без произвольных путей файловой системы и сетевых сервисов.

### 2. Планирование последовательных ходов ИИ-героя
`FAIHeroesRoutePlanner::Plan` получает неизменяемый снимок `FAIHeroesAdvisorSnapshot`. На уровне `TACTICAL PLANNER` (ProtocolTier 3) перебирает допустимые варианты до **двух разных узлов**, проверяет `TravelCost` и бюджет движения, а для охраняемых узлов прогнозирует исход через тот же `FAIHeroesBattleEngine`, что использует сама игра. Для второго боя берёт **выжившую армию после первого**, а не первоначальный состав; учитывает полезность ресурсов, потери HP и стоимость движения. Выдаёт детерминированный маршрут или честное отсутствие безопасного. Движение и атаку выполняет **только игрок после подтверждения**.

Связка внедрена в `Source/AIHeroes/Private/AIHeroesAdvisor.cpp`: в глобальном совете при ProtocolTier 3 добавляется `ROUTE: ...`. Планирование не вызывает LLM и не симулирует реальные 1B/3B/8B веса.

## Проверка источников

- SoT30 v5.5.7: `SOT30_INTEGRITY_PASS`, Knowledge **30/30**, SHA256SUMS **32/32**, всего **4 054 425** байт. Закреплённый manifest SHA-256: `0decd118a6300324b757d2587ec315a569aaf5800299d2b33bf046feaf56aeff`.
- Первый Editor-build блокировал `Live Coding` от уже работающего окна — исходники не были причиной.
- Первая Game-компиляция через UBA блокировалась из-за нехватки commit memory/pagefile; системные настройки не менялись.
- Повторная команда: `dotnet .../UnrealBuildTool.dll AIHeroes Win64 Development -Project=C:\github\AIHEROES\AIHeroes.uproject -WaitMutex -NoHotReload -NoUBA -MaxParallelActions=1`. **PASS**, 29/29 действий, `Result: Succeeded`.
- Editor UBT, параллельная сборка другого локального процесса: **PASS**. Наш повторный вызов дал `Target is up to date`, `Result: Succeeded`.
- Автотесты: отдельно наблюдены два журналируемых Unreal Automation Framework запуска `Automation RunTests AIHeroes`, каждый **22 `Test Completed`, 22 `Success`, 0 ошибок**, `Automation Test Queue Empty 22 tests performed`. Обе автоматизации запускались локальными процессами; третью одновременно не запускали во избежание коллизий.
- Дополнительная независимая 3D-QA телеметрия (другой игровой процесс, не наш ввод): `Saved/Logs/AIHeroesReplayQA.log` содержит `FOREST CAPTURED`, `[AI HEROES 3D] START node=FOREST, verified_actions=3`, три последовательных шага `REPLAY 1/3..3/3`, `SKIP requested` и `[AI HEROES 3D] FINISH; camera restored; strategic state unchanged`. Созданы `Saved/Screenshots/AIHeroes3DReplayQA.png` (463960 байт) и `AIHeroesAfterReplayQA.png` (385939 байт). Журнал подтверждает работу событий, но наша сессия не выполняла визуальное инспектирование пикселей.
- Среди 22: `AIHeroes.Advisor.TwoStopRoute`, `RouteBudgetAndAuthority`, `RoutePostCombatSurvivors`; `AIHeroes.Campaign.RotatingSaveRecovery`, `LegacySaveMigration`, `RejectedSaveKeepsBackup`, `MultiSlotIsolation`; `AIHeroes.Replay.DeterministicVerification`, `DiskRoundTrip`, плюс ранее существующие регрессии.
- `git diff --check`: exit code 0. LF/CRLF предупреждения локальных файлов не являются ошибкой компиляции.

## Подтверждённые файлы: байты | SHA-256

| Путь от корня AIHEROES | Байты | SHA-256 |
|---|---:|---|
| `Source/AIHeroes/Public/AIHeroesRoutePlanner.h` | 669 | `b1f8eda4fa2d7b58acaf59e4347de526101138e35467b1583f5c5a090c090643` |
| `Source/AIHeroes/Private/AIHeroesRoutePlanner.cpp` | 5755 | `0ca25b216d07489b53c04753a42042bab351b08611860d88a2019e907fbe5dcd` |
| `Source/AIHeroes/Private/Tests/AIHeroesRoutePlannerTests.cpp` | 5091 | `825d4eb6b8b60a05e1065f94236300b923d08960bf65e9391d2d018f9624727c` |
| `Source/AIHeroes/Public/AIHeroesSaveSystem.h` | 1090 | `272e2ebc19d1c2e578c7470b75e4c08980dcf6d511d74a13725093f3b0282f57` |
| `Source/AIHeroes/Private/AIHeroesSaveSystem.cpp` | 7467 | `f9b3537c5d3a69cd5fe34dc805528a60c8af88c1dee089a623c42901d7bbe031` |
| `Source/AIHeroes/Private/Tests/AIHeroesSaveSystemTests.cpp` | 9939 | `17cb876bce57f448e01f6a30f3ab8d6f281c8e172e8930179a6fd5da3b51a616` |
| `Source/AIHeroes/Public/AIHeroesCampaignSave.h` | 2076 | `3d113ea19364ebb4454cc97399c14299397ebcf4291a55adff9a8b0ffaccd152` |
| `Source/AIHeroes/Private/AIHeroesAdvisor.cpp` | 6421 | `e29a6152112518e18447231b5d538852f366f84a184dd8bb4833c9a7568ab2c1` |
| `Binaries/Win64/AIHeroes.exe` | 332206080 | `7f1945b0baa00c1df6937d4e5c986391c53f025706c1e2e78338f29e0f7f8bc5` |
| `Binaries/Win64/UnrealEditor-AIHeroes.dll` | 563712 | `e7429ed9fd0209947e461b83beb356280eb025dc9b16c66d5a58b47e64d6fb85` |

Проверенные журналы при read-back (внутри `Saved/Logs`, Git-ignored):
- `AIHeroesReplayTests.log` — 282372 байта, SHA-256 `f2a75147812b9ecbe8c853ba0a41b1b6ecc82ee17ed7dae978f150e4af88154d`, 22/22 PASS.
- `AIHeroes.log` — 277387 байт, SHA-256 `d0bb833a4fbe1d3f88e2e352d9ca7482b4a34ced4fd29c9dfac974938bc1c42d`, 22/22 PASS.

## Ограничения, конфликты и следующий критерий

1. **Новый интерактивный 3D-проход не выполнен именно нашей сессией.** На ПК одновременно открывались игровые QA-окна другими процессами. Мы не посылали в них ввод, не перезаписывали пользовательские сохранения и не выдаём автоматические тесты за визуальный acceptance.
2. **Параллельная разработка:** сторонние в текущем процессе изменения BattleStage, Replay, HUD и трёх кампаний были сохранены; это не результаты исключительно нашего изменения. `FAIHeroesSaveRepository` появился отдельно, но текущий активный `GameMode` работает через `FAIHeroesSaveSystem`. Перед объединением веток нужен один основной контракт.
3. **Совет о маршруте** появляется в глобальном запросе без выбранного узла; после выбора узла личный прогноз имеет приоритет. Стоит выделить отдельную кнопку «ПЛАН» и проверить работу в ручном UI-тесте.
4. **Android:** пока не собран APK. Ранее отсутствовали Android SDK Platform 35 и NDK r27c; требуется отдельная проверка SDK, сборка и тест на телефоне.
5. **ИИ-модель:** реальные 1B/3B/8B чекпойнты, бенчмарки и типизированный LLM-адаптер НЕ внедрены. Код использует детерминированную политику боя, а обновления протоколов являются игровыми способностями.
6. **Жизненный цикл:** изменения **локальные, сборка и тесты PASS**, но нет коммита/пуша/деплоя/Android release. Разработчики одновременно меняют один worktree; окончательный merge/release требует заморозки и новой полной проверки.

**Итог:** `LOCAL_PC_COMPILE_PASS | AUTOMATION_22_22_PASS | VISUAL_QA_PENDING | ANDROID_BLOCKED | LLM_NOT_IMPLEMENTED`.

**∆DΩΛ:** ∆ — резервируемые сохранения + стратегический совет из двух последовательных действий; D — исходники и тесты, подтверждённые UBT 29/29 и 22/22 automation; Ω — 95% для указанных проверок, ниже для состояния незапущенного в этой сессии UI; Λ — повторить после остановки параллельных QA и визуальной проверки, затем Android/LLM-этап.
