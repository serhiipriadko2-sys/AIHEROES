# AI HEROES — интеграционный gate GridBattle V2 / live 3D, 2026-10-28

Статус на момент этого read-back: BUILD_PASS / FULL_AUTOMATION_FAIL /
TEST_PATCH_PENDING_REBUILD / LIVE_GAME_QA_NOT_RUN.
Поверхность: C:\github\AIHEROES. Этот документ описывает новые
параллельные изменения без смешения с ранее проверенным 3D replay v1.

## Пройденный gate сборки

- В общей рабочей копии появились FAIHeroesGridBattle, 8x6 grid,
  пошаговая сессия, события Move/Attack/Wait, AAIHeroesLiveBattleStage
  и новая ответственность GameMode: commit кампании только после
  детерминированного окончания и верификации записи боя.
- Исходный blocker UHT (AAIHeroesLiveBattleStage отсутствовал) исчез,
  когда соседний процесс создал оба файла этого класса.
- Первоначальная unity ошибка C2084 из-за одинакового TotalHP
  устранена минимальным переименованием helper в RouteTotalHP
  только в AIHeroesRoutePlanner.cpp, математическая формула прежняя.
  Прежний файл сохранён под Saved\IskraBackups\UnityNameCollision_20261028.
- Новая сборка с 21 C++ действиями компилировалась, но не
  слинковалась из-за DLL lock окна AIHeroesRouteQA.log.
  Этот завершённый, неактивный -game QA процесс был идентифицирован
  по командной строке и штатно закрыт CloseMainWindow, не force-kill.
- Затем проведена ПОСЛЕДОВАТЕЛЬНАЯ компиляция UBT с
  -NoUBA -MaxParallelActions=1; 11 действий, link DLL/lib успешен,
  Result: Succeeded, процесс завершился с exit code 0.
  Низкая память/переполненный файл подкачки остаются эксплуатационным
  риском: многопоточная UBA сборка давала VirtualAlloc failure.

## Обнаруженный тестовый дефект

Команда UnrealEditor-Cmd с -ExecCmds="Automation RunTests AIHeroes"
на обновлённой DLL в логе
Saved\Logs\AIHeroesLiveIntegrationFinalTests.log выявила CRASH,
а НЕ успешную группу тестов. На стадии теста
AIHeroes.Campaign.GridGarrisonCompatibility, исходный файл
Source\AIHeroes\Private\Tests\AIHeroesBattleRecordTests.cpp:202,
Unreal TArray отказывает в добавлении элемента из собственного буфера
Outpost.Defenders.Add(Outpost.Defenders[0]).
Это alias/invalidation assertion, в логе Unreal Array.h:2196:
"Attempting to use a container element which already comes from the
container being modified". Сводного PASS для нового live GridBattle нет.

Изменение после read/backup:
  FAIHeroesArmyStack ThirdDefender = Outpost.Defenders[0];
  Outpost.Defenders.Add(ThirdDefender);
Перед кодом оставлен комментарий о правиле TArray.
Это сохраняет исходную проверку: третий нейтральный отряд запрещён.
Backup (в gitignored Saved): 11059 байт;
SHA256 5f0974394a7bc5eb8b3bd25be4f5ddefd9b19b43d6f2c8cc7c156067f18c704a.
Исправленный исходник: 11198 байт;
SHA256 c2a605f2c676b45abb4d398f01dd8c7d65acac649860ae3c92641be1ae618639.

ПОВТОРНАЯ СБОРКА/ТЕСТ ПОСЛЕ ПАТЧА НЕ ЗАВЕРШЕНЫ: другая
авторизованная сборка проекта в Saved\RecruitQA_20261028_220949
уже удерживала Global UnrealBuildTool Mutex. Ожидающий повтор
НАШЕЙ сборки был остановлен; чужой процесс не завершался.
Таким образом fix state = edited, syntactic diff checked,
not compiled and not test-verified. Никакой fake PASS.

## Параллельно подтверждённая рабочая база

- В предыдущей версии runtime 3D replay V1: build PASS, automation
  22/22 PASS, а window-only smoke показал FOREST selection, решение
  игрока атаковать, битву ИИ, трёхмерную сцену, поэтапные атаки,
  SKIP и восстановление камеры. Поздний screenshot показывает
  контрастные панели HP двух армий в 3D.
- Отчёт и фото: Docs\BUILD_RECEIPT_3D_REPLAY_2026-10-28.md,
  Docs\AIHEROES_3D_REPLAY_INTERACTIVE_2026-10-28.png,
  Docs\AIHEROES_3D_REPLAY_RETURN_2026-10-28.png,
  Docs\AIHEROES_3D_REPLAY_LIT_2026-10-28.png.
- Важно: это QA для старой архитектуры replay v1.
  НОВАЯ live grid stage не прошла ручное прохождение и тестовый gate.

## Контрольные файлы / байты / SHA256

| Путь от C:\github\AIHEROES | Байты | SHA256 |
|---|---:|---|
| Binaries/Win64/UnrealEditor-AIHeroes.dll | 805888 | c14106026ff034eaebdabcdce33cda1465e93b26ac962ee097baa0e458f2deb4 |
| Source/AIHeroes/Private/Tests/AIHeroesBattleRecordTests.cpp | 11198 | c2a605f2c676b45abb4d398f01dd8c7d65acac649860ae3c92641be1ae618639 |
| Docs/3D_BATTLE_REPLAY_CONTRACT.md | 6322 | 3ca6f1ec251551e1ed478769297ed52d639aacd4d22f71f90e3a8fdc6cfff804 |
| Docs/AIHEROES_3D_REPLAY_INTERACTIVE_2026-10-28.png | 463960 | da8e142cd44655c1e89215141d788b75c60f7b16d01f4a9b87d768538b5206b5 |

Этот binary был собран ДО исправления опасного TArray::Add;
пост-патч бинарник не предоставлялся. Hash/size = file evidence,
не доказательство test PASS на новых файлах.

## Дальнейший проверяемый gate

1. После окончания чужого UBT процесса выполнить
   dotnet UnrealBuildTool.dll AIHeroesEditor Win64 Development
   -Project=C:\github\AIHEROES\AIHeroes.uproject
   -WaitMutex -NoHotReload -NoUBA -MaxParallelActions=1.
2. Выполнить Automation RunTests AIHeroes с новым отдельным логом,
   требовать 100% Success и отсутствие Assert/Crash.
3. Провести реальный -game live battle: click forest, confirm,
   удостовериться, что campagn fingerprint неизменен до COMMIT,
   pause/speed, finish, результат применяется ОДИН раз.
4. Проверить replay/save/load после живого боя, аварийную отмену и
   возврат на карту, включая недоступность стратегических действий.
5. Затем Android SDK/NDK, мобильную производительность и typed LLM adapter
   отдельными gate без допущения, что слот 3B уже является LLM.

## Не заявлено

- Нет Android APK, live inference 1B/3B/8B или релиза.
- Нет доказательства полной работы live GridBattle на экране.
- Этот оператор не делал Git commit, push, merge или deploy.
- Метрики производительности и энергорасхода: computation unavailable.

PASS: предыдущая 3D replay v1 и новая pre-patch C++ сборка.
PARTIAL: тестовый TArray alias исправлен в исходнике, но post-fix gate не пройден.
FAIL: полный live-grid automation до fix падал на TArray alias.
BLOCKED: новый UBT mutex удержан соседней QA сборкой.

∆DΩΛ:
∆: улучшенная live-grid интеграция и локализованная причина crash в тесте.
D: build -> crash stack -> exact source -> backup -> surgical patch -> read-back.
Ω: 95% для доказанной причины и проверенного patch; post-fix тест UNKNOWN.
Λ: обновить статус после пост-патч полной сборки и успешной проверки live battle.
