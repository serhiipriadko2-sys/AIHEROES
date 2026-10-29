# AIHEROES — читаемое русское досье кампании и героя

Дата: 29 октября 2026 года. Ветка: `main`; базовый HEAD: `40049ea`.
Статус: **SOURCE_IMPLEMENTED / STATIC_QA_PASS / NATIVE_BUILD_NOT_RUN**.
Цель: сделать текущую основную кампанию понятнее, не создавая новых игровых правил.

## Проверенное состояние

В `AIHeroesAdvisorDossierHUD.cpp` отчёт отображался с английскими
статусами CURRENT SNAPSHOT / STALE / PREV / NEXT / REFRESH / CLOSE.
Оценка ширины строки использовала только ASCII 32–126, хотя досье
основной кампании уже содержит кириллицу.

Другой параллельный процесс в том же рабочем дереве занимается длинными
маршрутами, резервным боевым HUD и экраном результата. Эти ветви
в этом проходе не переписывались.

## Реализация

- Досье выводит «ЦЕЛИ КАМПАНИИ», «СОВЕТ ПО АРМИИ» или
  «СРАВНЕНИЕ ТАКТИК» в соответствии с существующими категориями.
  Для неизвестной категории исходный заголовок сохраняется дословно.
- Отдельно различаются «ДАННЫЕ АКТУАЛЬНЫ» и
  «ДАННЫЕ УСТАРЕЛИ — ОБНОВИТЕ».
- Все четыре действия подписаны по-русски:
  «НАЗАД», «ДАЛЕЕ», «ОБНОВИТЬ», «ЗАКРЫТЬ».
- Верхний заголовок, статусы и подписи имеют ограничение
  по **реально измеренной ширине** текста шрифтом Unreal Canvas.
- Для полного многостраничного отчёта консервативная ширина символа
  измеряется на латинице, кириллице, «Ё/ё», кавычках, тире и №.
  Сами строки отчёта остаются без усечения: сохранён прежний `Wrap`,
  страницы и передача информации из игрового GameMode.
- Добавлен `AIHeroes.Dossier.RussianCopyAndUnicode`: тест
  русских названий, флага устаревших сведений, четырёх кнопок,
  длинного кириллического слова и сохранности его содержимого.
- В открытом VS Code появилась задача
  `AIHEROES: Check Russian dossier (read-only)`.
  Вызывает `python -B Tools/verify_dossier_russian_ui.py`.

## Не менялось

Покупки, маршруты, туман войны, боевые правила, накопление опыта,
объекты мира, SaveGame, действие советника и проверка кликов поверх
модального окна. Заголовки и отчёт остаются разделены:
локализованная надпись **не изменяет источник информации**.

Полная интернационализация всё ещё требует перехода
на `FText` / строковые таблицы. Добавлен русскоязычный прототип,
не заявляется готовность всех языков или идеальная читаемость
любого символа/шрифта. Фактическая пиксельная QA пока не выполнена.

## Статическая квитанция: 6 файлов

| Файл | Байты | SHA-256 |
|---|---:|---|
| Source/AIHeroes/Public/AIHeroesAdvisorDossier.h | 2129 | 6040f22997ba2e865e191182f22250a0704bae7bc068286f03cc292d69e4d3d8 |
| Source/AIHeroes/Private/AIHeroesAdvisorDossier.cpp | 4951 | 633e06e04f0ab6ae605885a0b0bcad609302c4fdbd5cb737ef90783207583952 |
| Source/AIHeroes/Private/AIHeroesAdvisorDossierHUD.cpp | 5150 | f181cac4f994ceeddac01c8c989a3d6d24c5f17a7a9b701ef33e323671d1a06a |
| Source/AIHeroes/Private/Tests/AIHeroesAdvisorDossierTests.cpp | 11276 | f2beef04cbf56cf684052990490850a33b42cc444ab93a259fe9ca9a91d1feea |
| Tools/verify_dossier_russian_ui.py | 4818 | 790e1100d9d87c6a5b28796a4bfe72978f160eef818db7d71368ec4094d03d26 |
| .vscode/tasks.json | 8106 | 9d2d80ef551f9a12c48a6288a40e4880288c5547c88a648fe7307fd05a07de5c |

Проверки: 6/6 SHA/размеров, UTF-8 replacement chars=0, trailing whitespace=0,
`git diff --check` без ошибок форматирования.
`python -B Tools/verify_dossier_russian_ui.py`:
**12/12 PASS; STATIC_PASS_NATIVE_NOT_RUN**.
Сверка начального и текущего `FAIHeroesAdvisorDossierRules::Wrap`:
**содержимое функции неизменно**.

Ни один из этих результатов не доказывает компиляцию, что видно и
по отсутствующей свежей квитанции `Docs/AIHEROES_BUILD_LATEST.json`.
Предыдущие native build попытки сталкивались с нехваткой памяти;
параллельные source edits ещё не зафиксированы в общем снимке.

## Приёмка в VS Code и Unreal

1. Открыть `.vscode/tasks.json` и вызвать `Tasks: Run Task` →
   `AIHEROES: Check Russian dossier (read-only)`.
2. После стабилизации исходников — собрать `AIHeroesEditor Win64 Development`
   с новым хеш-манифестом.
3. Выполнить `AIHeroes.Dossier.RussianCopyAndUnicode` и весь `AIHeroes`
   на этой же DLL, без ошибок и предупреждений.
4. Открыть «Задачи», «Совет по армии», «Отчёт героя», «Тактики»
   в игре на 1280x720, 960x720 и 640x720.
5. Проверить полноту кириллических строк и навигацию по страницам,
   отсутствие перекрытия соседних кнопок, корректность актуальности
   после изменения ресурсов, нулевой дрейф пользовательских saves.
6. Только после этого ставить `GAME_UI_PASS`.

## Откат

Базовые копии четырёх Source-файлов:
`Saved/QA/DossierCyrillic_20261029T2150/*.before`.
Откат — **только собственных hunks**, после повторного чтения и
сверки хеша. Из `.vscode/tasks.json` удалять только добавленную задачу.
Не выполнять `git reset --hard`, `git clean`, не удалять пользовательские
сохранения и не завершать чужие процессы.

## Внешние ориентиры

- https://dev.epicgames.com/documentation/en-us/unreal-engine/automation-test-framework-in-unreal-engine
- https://dev.epicgames.com/documentation/unreal-engine/text-localization-in-unreal-engine
- https://dev.epicgames.com/documentation/unreal-engine/scalability-and-the-developer-for-unreal-engine

ΔDΩΛ: Δ — русские подписи и корректнее измеряемые строки досье;
D — six-file source receipt, 12 static checks, original Wrap diff;
Ω — 95% для файловой целостности, runtime UNKNOWN;
Λ — source-pinned build, automation, реальные кадры и ручное прохождение.
