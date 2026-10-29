# AIHEROES — понятные потери после живого боя

Дата: 29 октября 2026 года.
Статус: SOURCE_IMPLEMENTED / STATIC_QA_PASS / NATIVE_BUILD_INCOMPLETE.
Основа: существующая основная кампания AIHeroesGameMode, Grid V2.

## Задача игрока

После завершения 3D-автобоя игрок должен сразу видеть, что изменилось в его армии,
и понимать, как вернуться к стратегической карте. До этой правки нижняя строка HUD
продолжала перечислять управление камерой даже на экране результата.

Теперь только в фазе Result выводится короткий итог по сохранённой записи боя,
например: «На карту — продолжить. Бойцов: 7 из 11; потери: 4».
Во время продолжающегося или приостановленного боя прежние подсказки сохраняются.
Первое слово на узком экране — «На карту»: при обрезании текста остаётся
понятное следующее действие.

## Изменения

- Source/AIHeroes/Public/AIHeroesBattleAftermath.h — чистое представление результата.
- Source/AIHeroes/Private/AIHeroesBattleAftermath.cpp — подсчёт выживших и потерь.
- Source/AIHeroes/Private/Tests/AIHeroesBattleAftermathTests.cpp — четыре теста.
- Source/AIHeroes/Private/AIHeroesBattleHUD.cpp — одна интеграция только для Result.

Расчёт использует PlayerBefore и PlayerAfter из LastBattleRecord и численность
по TotalHealth/UnitHealth с 64-битной промежуточной арифметикой. Неполное
здоровье живого бойца не считается гибелью ещё одного бойца.

## Границы безопасности

- Только отображение. Ничего не выдаёт, не списывает и не сохраняет.
- Не показывает сведения о вражеской армии; даже подмена EnemyBefore/EnemyAfter
  не влияет на строку потерь своих войск.
- Не угадывает сумму награды: исторический battle record не хранит достоверный
  факт начисления ресурсов отдельным полем. Экономика остаётся авторитетом.
- Не меняет боевую политику ИИ, V1/V2, маршрут, туман войны, SaveGame.
- При пустом/противоречивом результате, отрицательном HP, невозможной численности
  или неизвестной записи показывается честная строка «Данные о потерях недоступны».
- Вызов возможен на экране результата, после CommitLiveBattle, а не во время боя.

## Написанные тесты Unreal (ещё не исполненные как полная группа)

1. AIHeroes.Playability.BattleAftermath.VictoryAndWounded
2. AIHeroes.Playability.BattleAftermath.DefeatAndNoSurvivors
3. AIHeroes.Playability.BattleAftermath.InvalidFailsClosed
4. AIHeroes.Playability.BattleAftermath.NoEnemyOrRewardLeak

Локальная исходная проверка: 12/12 PASS; отрицательный пробный сценарий
замены источника своими/чужими отрядами отклоняется; git diff --check PASS.
Это статические проверки, а не доказательство успешной сборки.

## Состояние нативной компиляции

В параллельном рабочем потоке уже были запущены:
- Docs/BuildRuns/20261029T172754393Z/build.log — C3859/C1076, недостаточно
  виртуальной памяти Windows при создании PCH.
- Docs/BuildRuns/20261029T172858033Z/build.log — C1060 при компиляции тестов
  Unreal; файл AIHeroesBattleAftermath.cpp до этого присутствовал в списке
  отдельных компилируемых единиц.

Полный AIHeroesEditor Win64 Development не собран: результат FAILED,
поэтому никакого нового Automation PASS, GPU-визуала или end-to-end прохождения
этим этапом не заявляется. Нельзя восстанавливать прежний BUILD_LATEST
как будто он относится к текущей версии.

## Проверка в Unreal после стабилизации общего рабочего дерева

1. Обеспечить достаточно свободной виртуальной памяти без принудительного
   закрытия чужих процессов и без изменения системных настроек из этого среза.
2. Запустить Tools/build_aiheroes_editor.ps1: получить свежий compiled.json
   с неизменными хешами всех входных файлов.
3. Запустить Tools/run_aiheroes_automation.ps1 с -Filter
   AIHeroes.Playability.BattleAftermath и -CompiledSnapshot с путём к свежему
   compiled.json; затем всю группу AIHeroes на том же снимке.
4. Захватить настоящие кадры результата победы и поражения при 1280x720 и
   640x720; проверить, что строка не закрывает кнопку «На карту».
5. Пройти единый путь: стратегическая карта → подтверждение нападения →
   автобой → потери → возвращение на карту → пополнение и сохранение.

## VS Code, откат, внешние ориентиры

Правки можно наблюдать в открытом VS Code: окно AIHEROES подтверждено,
AIHeroesBattleHUD.cpp открыт командой code --reuse-window --goto.
Копия старого файла:
Saved/QA/PlayableBattleAftermath_20261029/AIHeroesBattleHUD.cpp.before
с SHA-256 553bd5186bfa31e9ec9aac0513743ca1ed07e6b000b65bc21a4899be62eae8e0.
Откат — только собственного хунка HUD и трёх созданных файлов после сверки
их актуальных хешей, не использовать git reset/clean или принудительное
завершение чужих редакторов и задач.

Источники:
- Epic Automation Test Framework:
  https://dev.epicgames.com/documentation/unreal-engine/automation-test-framework-in-unreal-engine
- Epic Review Test Results:
  https://dev.epicgames.com/documentation/unreal-engine/review-test-results-in-unreal-engine
- VS Code CLI --reuse-window/--goto:
  https://code.visualstudio.com/docs/configure/command-line

Delta: результат боёв стал объяснимее без изменения правил и наград.
D: source read-back + проверка интерфейса + тесты + проверка ложного ввода.
Omega: высокая уверенность для статического результата, неизвестна для runtime.
Lambda: пересмотр после полной нативной компиляции и реальных игровых кадров.
