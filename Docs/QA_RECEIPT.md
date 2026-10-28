# AI HEROES v0.1 — локальная квитанция проверки

- Дата: 2026-10-28
- Surface: Remote Desktop Commander / Windows desktop `DESKTOP-ITUAMNF`
- Папка: `C:\github\AIHEROES`
- Область: Unreal 5.8.4, Windows editor/game, C++ исходники и локальная визуальная проверка
- Статус: **PC PROTOTYPE TESTED-LOCALLY**, не Android release и не production

## Проверки

| Gate | Результат | Свидетельство |
|---|---|---|
| Сборка `AIHeroesEditor Win64 Development` | PASS | UnrealBuildTool `Result: Succeeded` после финального изменения HUD |
| Unreal Automation тесты | PASS 6/6 (повторная проверка обновлённого кода) | `Automation RunTests AIHeroes`: `AtomicResourceSpend`, `DeterministicBattle`, `NoArmyCannotWin`, `ProtocolChangesTactics`, `TravelBudget`, `CombatPolicy` |
| Запуск 3D-мира | PASS | `LogWorld: Bringing World /Engine/Maps/Entry.Entry up for play` |
| Интерфейс/камера | PASS (visual smoke) | Скриншот `Docs/PROTOTYPE_SCREENSHOT.png`, 1296 × 759 |
| Улучшение героя | PASS (input smoke) | Нажатие P через окно Unreal; лог `Unlocked THREAT ANALYSIS` |
| Совет ИИ | PASS (visual smoke) | Нажатие H; показан `Threat scan: FOREST. Forecast: likely win` |
| Автоматический бой и захват | PASS (runtime smoke, повторно после обновления) | P → выбор FOREST кликом → H → Enter; `FOREST CAPTURED`, `VICTORY | 1 rounds | your units: 11 | enemy units: 0`; 3 строки атак в логе |
| Android toolchain | BLOCKED | Unreal SDK validation: `Android INVALID r27c` |

## Скриншоты и контрольные суммы

- `Docs/PROTOTYPE_SCREENSHOT.png` — **495928 bytes**, SHA-256 `AE40199B6CBFA078B95B6F344818E7175309E6F52198071CF984F76977D3B3B3`
- `Docs/BATTLE_RESULT.png` — **485094 bytes**, SHA-256 `B54B7BA2D6FEF7BE7AE55D1BFFB1E3C72919A3958106F8B8CB4165B4157C5840`
- `AIHeroes.uproject` — **318 bytes**, SHA-256 `16E606F3DD237EF5E686401A72B2E29766C7F45048309E9B74C86630E98BECEE`

## Границы и остаточные риски

- Реальные языковые модели 1B/3B/8B и загрузка файлов **ещё не подключены**. ИИ сейчас детерминированный, то есть основан на правилах.
- Художественная карта использует встроенные примитивы Unreal вместо объектов Blender.
- Android SDK/NDK не настроен; Android APK не создавался.
- Во время работы параллельный процесс создал начальный коммит `fcbb303` (`start`). Это НЕ моя операция. Я не выполнял commit/push/merge; правки GameMode и новые тесты после первичной проверки требуют повторного QA.
- В этой папке в тот же сеанс появились дополнительные классы `AIH*` (параллельная работа). Они сохранены; активный GameMode в `Config/DefaultEngine.ini` — `AIHeroesGameMode`. Перед дальнейшим расширением нужно выбрать одну архитектуру, не удаляя чужие изменения.

## Следующие проверки

1. Android Turnkey / NDK r27c и настоящий телефон.
2. Интеграция реальной языковой модели через валидируемый адаптер, без права изменять правила боя.
3. Сохранение партии, путь героя по карте и новая 3D-графика.

Квитанция подтверждает только перечисленные локальные действия. Не является подтверждением релиза или размещения в GitHub.
