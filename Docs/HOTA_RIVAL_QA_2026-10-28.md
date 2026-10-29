# AI HEROES — Horn of the Abyss research + Rival Outpost V1 QA receipt

Дата: 2026-10-28. Компьютер: DESKTOP-ITUAMNF. Рабочий проект: `C:\github\AIHEROES`.
Цель: системное исследование HotA 1.8.1 и новая оригинальная игровая подсистема AI HEROES.
Lifecycle: **researched → implemented → compiled → automated-tested → visual-smoke-tested locally**.
Не означает `committed`, `pushed`, `Android packaged`, `released`, `LLM loaded` или
`fully feature-complete`.

## A. Intake / B. SIFT — источники и границы

- Пин SoT30 v5.5.7: `SOT30_INTEGRITY_PASS`, Knowledge 30/30,
  SHA256SUMS 32/32, corpus_bytes 4054425; исходный канон не изменялся.
- [FACT] Официальная HotA **1.8.1**, 25 августа 2026:
  https://h3hota.com/en/download
- [FACT] Официальная HotA-документация — три города:
  Cove («Причал»), Factory («Фабрика»), Bulwark («Кронверк»).
  «Оплот» — Rampart из оригинальной Heroes III; не Bulwark.
  https://h3hota.com/en/documentation
- [FACT] RMG template defines connected map zones and protected passages:
  https://h3hota.com/en/template-format
- [FACT] Классические ежедневный доход, недельный рост армий и выбор
  стратегического маршрута документированы в Heroes III manual:
  https://www.staroceans.org/hero3/Restoration_of_Erathia_Manual
- [INTERP] Выбрана ценность **последовательности циклов** экономика →
  риск → бой → территория, а не буквальное копирование чужих фракций.
- [HYP] Календарь набегов, их стоимость и темп пополнения — временные
  авторские балансные параметры, пока без долгого плейтеста.
- **Полный системный разбор** (249 строк, все основные типы игровых систем,
  различия HotA и Heroes III, дизайн-matrix, roadmap, цитируемые ссылки):
  `Docs/HOTA_GAMEPLAY_RESEARCH_2026-10-28.md`.
  В репозитории параллельно появились два независимых анализа HotA 1.8.1;
  этот отчёт не присваивает их создание текущей операции.

## C. Frame — игровые правила реализованного среза

- **NPC «Налётчики форпоста»** не меняет стратегические команды человека,
  герой не превращается в реальный LLM и не отдаёт автономные приказы
  игроку. Единственная точка активации — разрешённый `SPACE/EndTurn`.
- После доходов выполняется детерминированный `WorldWeek` proposal,
  затем `RivalStrategy` proposal. **Весь следующий день валидируется
  до изменения authoritative Gold/Wood/Ore/Day/NodeStates**.
- При враждебном OUTPOST на дни 8,15,22... NPC может добавлять
  +2 Raider (+22 HP) поверх совместимого нейтрального недельного
  прироста; при совпадении дня с набегом вербовка приоритетнее.
- Каждые 3 дня возможен набег только на принадлежащую игроку шахту:
  не более 6 WOOD или не более 4 ORE, без ухода ресурсов ниже нуля.
  Направление выбирается детерминированно с fallback.
- Захват OUTPOST игроком отключает NPC-развитие и набеги.
- Все статы NPC проверяются; невалидная карта/гарнизон блокирует
  день без частичного расхода, нет несанкционированных боёв.
- Контракт `CurrentSchema == 1` сохранён: гарнизон и ресурсы уже
  входят в существующие поля SaveGame; новый произвольный файл памяти NPC
  не создавался.
- HUD показывает **предупреждение о будущем** риске отдельным текстом;
  фактический налёт фиксируется постфактум в событии и журнале.
- Обязательный фрагмент исходников: `AIHeroesRivalStrategy.h/.cpp`,
  `AIHeroesRivalGameMode.cpp`, `AIHeroesGameMode::EndTurn`, HUD, тесты.
- У существующего `AIHeroesWorldWeek` и `AIHeroesRecruitmentStock`
  были параллельные авторы. Их код не удалён, совместимость
  учтена, правки чужих модулей ограничены компиляционными конфликтами.

## D. Step — конкретные действия и сборка

- UBT: `Build.bat AIHeroesEditor Win64 Development
  -Project=C:\github\AIHEROES\AIHeroes.uproject
  -WaitMutex -NoHotReload -NoUBA -MaxParallelActions=1`: **PASS**.
- Первая совместная компиляция обнаружила две области конфликта:
  необозначенный namespace констант лимитов в RivalStrategy и
  одноимённую функцию `Rect` в HUDLayout. Они исправлены.
- Более поздняя unity-компиляция Unreal обнаружила два одинаковых
  `ValidResource` в `AIHeroesDevelopmentRules.cpp` и
  `AIHeroesBasePanel.cpp`. Локальная функция первого файла
  переименована в `ValidDevelopmentResource`; формулы не менялись,
  следующая сборка **PASS**.
- `Source/AIHeroes/Private/Tests/AIHeroesRivalStrategyTests.cpp`:
  четыре самостоятельных случая — рейды и authority,
  рост/захват, fail-closed, сериализация v1 — **4/4 Success**.
- Полный `Tools/run_aiheroes_automation.ps1 -Filter AIHeroes`:
  **98/98 Success, failed=0, notRun=0, warnings=0, exit 0**.
  Последний отчёт: `Docs/AutomationRuns/20261028T204442261Z/index.json`.
- После сборки C++-исходников новее DLL не было на момент
  проверки: `SOURCE_NEWER_THAN_DLL=0`.
- `git diff --check` — exit 0. Никакой commit/push/deploy не выполнялся.

## E. Verify — реальный Windows playthrough и отдельные evidence

| Проверка | Факт |
|---|---|
| Day 1→7 в собственном окне | 6 подтверждённых EndTurn, без автокоманд |
| Day 7→8 | Лог `[AI HEROES WEEK] week=2 day=8 recruits=7` |
| Day 8 OUTPOST | Лог `[AI HEROES RIVAL] day=8 action=1 amount=22` |
| Игрок разрешил бой за FOREST | `[AI HEROES LIVE] BEGIN`, затем `COMMIT`, победа за 2 раунда |
| Доход захваченного леса | День 2: WOOD=50, +15 обычного дохода |
| Прогноз дня 3 | Предупреждает о 6 WOOD, которые рискует потерять игрок |
| Day 3 RAID | Лог `action=2 amount=6 wood=59 ore=30 player_order=0` |
| Проверка сохранений | Script сравнил число файлов и SHA256 до/после, все без изменений |
| Результат 7→8 | `RIVAL_WEEKLY_INTEGRATION_PASS no_save_writes=1` |
| Результат захвата+набега | `RIVAL_AUTHORIZED_CAPTURE_AND_RAID_PASS day3_wood=59 no_save_writes=1` |
| Фраза будущего времени | Дополнительная визуальная проверка `RIVAL_FUTURE_FORECAST_QA_PASS`, day 7 |
| Android/APK, реальный LLM | НЕ ПРОВЕРЕНО и НЕ РЕАЛИЗОВАНО |

Дополнительный smoke текста предупреждения начал с нестабильной
серии быстрых `PostMessage`: один день не зарегистрировался.
QA-скрипт ограниченно повторяет пропущенный ввод и проверяет реальные
переходы дней по журналу, а не принимает нажатия за успех. После
изменения тест **PASS**. Глобальные нажатия не применялись.

## F. Concrete artifact receipts

| Путь относительно `C:\github\AIHEROES` | Размер, байт | SHA256 |
|---|---:|---|
| `Docs/HOTA_GAMEPLAY_RESEARCH_2026-10-28.md` | 30528 | `A7B3382EF053495555F84AAB287419DF0E383C6260CDE71CE4E5F465CDA00B2C` |
| `Docs/RIVAL_FACTION_V1.md` | 7288 | `2113025B1AA2160581528663F9479C235B0ADF79949F12BC8A972B559F1E8525` |
| `Source/AIHeroes/Public/AIHeroesRivalStrategy.h` | 1614 | `9AC5B38CB2EF94A12BF0CBDDB1C6FE94694336505DA674C9ACC67F275FE2D5CC` |
| `Source/AIHeroes/Private/AIHeroesRivalStrategy.cpp` | 6489 | `4829ACF1FA12D582E361B447D98ECE1F7427CE25B1D72377BDA355D19ACFE76A` |
| `Source/AIHeroes/Private/AIHeroesRivalGameMode.cpp` | 5361 | `6FDA9E4A34E53324928D34FDAD6347785027B016872FB9AA449F8D41780FB934` |
| `Source/AIHeroes/Private/Tests/AIHeroesRivalStrategyTests.cpp` | 9145 | `0E4295D763BDCBDC0BFF132D84399D170179A4C0D5AD71F2DB04A1C0667D1EAF` |
| `Tools/qa_rival_turns.ps1` | 3292 | `7ADD5295B61F157DCCC1147FD1DE3A932CCD9F484FA91976517DDF6B192E2236` |
| `Tools/qa_rival_raid.ps1` | 4097 | `64E996AAE25F792F7AFAE46C3ED15616E9726FC04C9B7DCC43AEE9C2F3629B39` |
| `Docs/RIVAL_PRE_WEEK_QA_2026-10-28.png` | 357869 | `1D8012BB87BB75E9203E1D6941B4DAE29C296A04C9385F27553998DFEFA9ADA4` |
| `Docs/RIVAL_WEEK_TWO_QA_2026-10-28.png` | 359666 | `C9DC72728D2B5C30D719F0D80CD7D90FF0D42D91292700D88B5DBAB128F18E5D` |
| `Docs/RIVAL_FOREST_CAPTURE_QA_2026-10-28.png` | 356069 | `908EFBCA90DA3A8EFEDDC9FA58CDE605E3E7AEC20DA5155E8DAC56BADF24CD50` |
| `Docs/RIVAL_RAID_FORECAST_QA_2026-10-28.png` | 360789 | `639B41AD00161770F57ACD92C21B95511B3699B2BD397BC391CCBCC8F459EBA5` |
| `Docs/RIVAL_RAID_APPLIED_QA_2026-10-28.png` | 359174 | `36FCD395B2B574D5E8DDB4DFCA5E63D41AAEA08679135ABB33B5EE63FB60F314` |
| `Docs/AutomationRuns/20261028T204442261Z/index.json` | 38670 | `4A67D5ADE5274B2A164DFD363E778931F6D9CA61F94096147BEE0828EF805071` |

Поздний `Tools/qa_rival_forecast_v2.ps1` и
`Docs/RIVAL_FORECAST_FUTURE_TENSE_QA_2026-10-28.png` имеют
отдельные контрольные суммы в итоговой инвентаризации.

## Non-claims, risk and next step

- Это НЕ полный перенос HotA. Реальные фракции, 12 городов HotA,
  гексы, осады, сюжетные кампании, артефакты, магия, туман войны,
  генератор карт, глобальные NPC-армии, сетевой PvP и Android APK
  пока **не реализованы** в AI HEROES.
- Нет реальной локальной LLM-модели 1B/3B/8B, модельные слоты —
  игровые названия; тестирование на телефоне не проводилось.
- Параметры роста, доходов и рейдов требуют баланса на длинной
  кампании и тестов при полном захвате всех территорий.
- Рекомендуемый следующий независимый этап: карта из оригинального
  графа зон с туманом войны и охраняемыми проходами. Потом — XP и
  выбор специализации ИИ-героя, полноценные городские жилища,
  PvE-соперник с перемещаемой армией, осады. Сохранять atomic commit,
  typed proposals, контроль игрока и воспроизводимый replay.

## Addendum — интеграция после параллельного коммита

Позднее другой процесс опубликовал `cc062ba` и продолжил разработку
`AIHeroesTravelRules`, `AIHeroesZoneTemplate`, `AIHeroesRecruitmentStock`
и связанной экономики. Это **не мой commit/push**, исходники других
разработчиков не перезаписаны.

- Новая актуальная полная сборка UBT `AIHeroesEditor Win64 Development`
  прошла все **37/37 действий**, `Result: Succeeded`.
- Мой новый независимый `Tools/run_aiheroes_automation.ps1 -Filter AIHeroes`:
  **117 из 117 Success**, 0 Fail, 0 NotRun, 0 Warn, exit=0.
  `Docs/AutomationRuns/20261028T205131609Z/index.json`,
  **46 176 байт**,
  SHA256 `748C571E3B348D3EEC29689E7F1242E49B5F79D355802CDE7D44C6D700AF6F86`.
  Включены четыре `AIHeroes.Rival.*`, восемь `AIHeroes.TravelV2.*`
  и пять `AIHeroes.ZoneTemplate.*`.
- Прогноз HUD исправлен: теперь указывает на ещё не свершившийся риск.
  `Docs/RIVAL_FORECAST_FUTURE_TENSE_QA_2026-10-28.png`:
  **921 435 байт**,
  SHA256 `EA3A4D2BB4BB4606DECFDDD426C4D6E9A95809489F2CA62043842D4D76842794`.
  Скриншот показывает Day 7 и будущий +22 HP на Day 8.
  `Tools/qa_rival_forecast_v2.ps1`: **2 081 байт**,
  SHA256 `7CC620371324B5ABE69E7AE5365C8594BAD33AE9615B3159102A4B65784E8255`.
- К моменту контрольной проверки после **последней** сборки один
  параллельный исходник `AIHeroesTravelRules.cpp` получил новую дату
  изменения **23:51:47**, а DLL датирована **23:51:16**.
  Поэтому `117/117` честно подтверждает **собранный снимок до этой
  дополнительной правки**, а не обещает, что каждый байт последующего
  `working tree` уже проверен. Требуется повторить билд/QA, когда
  параллельные изменения стабилизируются.

**Итог: PASS — локально проверенный модуль налётчиков, research и
игровая QA / PARTIAL — актуальный после параллельных изменений
working tree, Android и готовность всей игры.**

∆DΩΛ: ∆ — активная фракция и системное исследование HotA;
D — локальная сборка, Unreal 98/98, 2 manual playthroughs,
read-back PNG/JSON и sha256; Ω — 93% для проверенной версии;
Λ — обновления параллельного кода, длительный плейтест и Android QA.
