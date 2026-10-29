# AIHEROES — AmberMarch Architecture V6

Дата: 2026-10-29. Статус: **AUTHORING_PASS / UNREAL_IMPORT_PENDING**.

## Что сделано

На основании трёх визуальных референсов пользователя продолжено оригинальное low-poly художественное направление AIHEROES. Ни один пиксель, модель, исходный ресурс или текстура из Heroes III / Horn of the Abyss не переносились в проект.

Новый набор содержит три оригинальных здания:

| Ресурс | Назначение | Силуэт |
|---|---|---|
| SM_CitadelCrownV6.fbx | HQ / цитадель Искры | Четыре башни, центральная крепость, синяя кровля, золотые навершия, ворота, флаги |
| SM_FrontierGateV6.fbx | OUTPOST / пограничная застава | Две башни, проход, решётка, бойницы, зубцы, сигнальный флаг |
| SM_ArchiveSanctumV6.fbx | RUINS / святилище | Круглая площадка, разрушенные колонны, бирюзовый кристалл, каменная арка |

Редактируемый Blender-источник: ArtSource/AmberMarch/ArchitectureV6/ArchitectureV6.blend.

В этой же папке три настоящие PNG-проекции (800×450): citadel_preview.png, frontier_preview.png, archive_preview.png.

## Инвентаризация и бюджет

- 3 оригинальных статических меша; всего **3309 треугольников**.
- Источник: Blender 5.2. FBX и authoring manifest содержат SHA-256 и размеры.
- Tools/build_amber_architecture_v6.py — исходный параметрический генератор.
- Tools/finalize_amber_architecture_v6.py — проверенное однократное восстановление после частичного экспорта.
- Tools/preview_amber_architecture_v6.py — низкопамятный CPU-предпросмотр.
- Tools/import_amber_architecture_v6.py — подготовленный, но ещё не выполнившийся Unreal Editor import.
- Tools/verify_amber_architecture_v6.py — manifest, bytes, SHA-256, размеры PNG и Python AST.
- Docs/ARCHITECTURE_V6_LOCAL_RECEIPT_2026-10-29.json — инвентаризационный receipt с 15 записями.

## Точечная интеграция в экспедицию

Изменён только визуальный слой Source/AIHeroes/Private/AIHeroesZoneExpeditionArt.cpp.

NodeMesh сначала проверяет доступность нового Unreal-ассета; если его нет, возвращает прежние SM_Hearth, SM_Watchtower или SM_Archive. Для V6 задаётся масштаб .43 внутри текущего Actor Scale; основная геометрия клика сохраняется. Тени предусмотрены лишь для немногих больших V6-объектов.

- Генератор дорог, правомерность боя и движение не менялись.
- Скрытые узлы и секретные гарнизоны не используются для оформления.
- Старые SaveGame и основная 5-узловая кампания не изменялись.
- Тест Source/AIHeroes/Private/Tests/AIHeroesArchitectureV6Tests.cpp проверяет контракт «0 или 3 из 3» optional meshes. Пока НЕ скомпилирован.
- Резервная копия затронутого C++ файла: Saved/QA/ArchitectureV6_20261029/AIHeroesZoneExpeditionArt.cpp.before. Откат только точечным diff с предварительным чтением.

## Что заблокировано и не объявлено успешным

Во время работы на компьютере был открыт UnrealEditor.exe — пользовательское приложение не закрывалось.

Отдельный UnrealEditor-Cmd.exe с Python commandlet завершился ошибкой Ran out of memory allocating 264 bytes (лог: Saved/QA/architecture_v6_unreal_import_engine.log). В системе оставалось примерно 1.3 ГБ доступного Virtual/Commit memory. Никакой из трёх ожидаемых V6 .uasset не создался. Это НЕ доказательство ошибки геометрии, но импорт и GPU-валидация игры не состоялись.

Первый Blender Eevee preview тоже не смог работать из-за ограниченной GPU-памяти. Использован Blender Cycles CPU: три предпросмотра созданы и проверены. Настройки подкачки и пользовательские процессы не изменялись.

## Следующий подтверждаемый переход

Когда проектный Unreal Editor будет закрыт штатным способом и доступной памяти будет достаточно:

1. Перепроверить git status, целевые .uasset и исходный FBX manifest.
2. Запустить Tools/import_amber_architecture_v6.py через Unreal Python commandlet один раз.
3. Проверить Saved/QA/architecture_v6_import_receipt.json и фактическую геометрию/материалы в UE.
4. Выполнить Tools/build_aiheroes_editor.ps1, затем Tools/run_aiheroes_automation.ps1 с CompiledSnapshot.
5. Запустить реальный визуальный QA на 1280x720 и 960x540, проверить появление V6 на карте и неизменность пользовательских сохранений.
6. Не заявлять live/Android готовность до отдельной проверки.

## Внешние основания

- Epic, FBX Static Mesh Pipeline: https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-static-mesh-pipeline-in-unreal-engine
- Epic, Scripting Unreal Editor Using Python: https://dev.epicgames.com/documentation/unreal-engine/scripting-the-unreal-editor-using-python
- Epic, Instanced Static Mesh Component: https://dev.epicgames.com/documentation/en-us/unreal-engine/instanced-static-mesh-component-in-unreal-engine
- Blender Python API migration: https://developer.blender.org/docs/release_notes/5.0/python_api/

## ΔDΩΛ

∆: три оригинальных архитектурных меша и подготовленная fallback-интеграция;
D: Blender, FBX, PNG, SHA-256/bytes, C++ код, лог блокирующего Unreal import;
Ω: 94% для проверенных authoring-артефактов; Unreal visual readiness UNKNOWN;
Λ: достаточная память, импорт, source-pinned сборка, автоматизация и GPU-проверка.
