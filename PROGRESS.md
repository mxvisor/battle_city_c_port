# PROGRESS.md — состояние проекта

Читать в начале сессии вместе с `git log --oneline -10`. Обновлять в конце сессии:
что сделано, что сломано, что дальше. Коротко; история — в git, детали сверки — в `PORTING.md`.

_Обновлено: 2026-09-27_

## Текущее состояние

- Все **218/218** top-level ASM-функций портированы и отмечены `[x]` в `PORTING.md`.
- Работает: title, construction, stage select, бой (1P/2P), demo, pts-screen, game over, hi-score, secret message; NTSC/PAL; SDL2 и SDL3.
- Регрессионный тест по кадрам (`tests/regression/run.sh`, `ctest`): сценарии `attract` (титул + демо с ИИ, 3600 кадров), `play_1p` (Stage 1 с вводом, пауза, Game Over, очки, 4800), `construction` (редактор + бой на своей карте, 1800). Эталон одинаков для Release/Debug/SDL3.
- CI (GitHub Actions) — сборка SDL2/SDL3 × Release/Debug с `-Werror` (кроме `unused-label`) + регрессионный тест + headless smoke-запуск на 10 с; отдельный job — регрессия под ASan/UBSan.

## Последние исправления (см. `git log`)

- Повторная сверка с ASM: ИИ (`battle_tank_status.c`), коллизии (`battle_collide.c`), звук (`sound_engine.c`, включая побайтную сверку 28 звуковых потоков и `Frequency_LUT`). Найдено и исправлено: перенос между двумя `ADC` в `Get_Random_A` (меняет ход боя — эталон обновлён); `Draw_Tile`/`Draw_TSABlock` — тайл/координаты через регистры, `STX Spr_X`, константа `$1C` вместо `PPU_Addr_Ptr`, без записи в `Spr_TileIndex`/`Block_X/Y`/`TSA_BlockNumber` (эталон не изменился).
- Запись за границу `SoundChannels[4]` (индекс до 245): render-поток вызывал `play_sound()` вне `nmi()` и до `Sound_Stop`, по power-on мусору. В ASM `Play_Sound` идёт только из NMI, а NMI — только при бите 7 `PPU_CTRL_REG1`; теперь этот вызов (`sdl_run.c`, `test_mode.c`) им и ограничен. Кадры не изменились, из потока APU ушли 20 мусорных записей при старте. Попутно: UB «сдвиг отрицательного» в `check_obj`. CI гоняет регрессию под ASan/UBSan.
- Добавлен детерминированный тестовый режим `--test-frames` (`src/test_mode.c`) и регрессионный тест по кадрам в CI.
- Неиспользуемые ASM-метки (18 → 5): ветвления `BPL`/`BEQ`/`BNE` в `battle_collide.c`, `battle_respawn.c` (`load_new_tank`, + пропущенные `@enemiesLeft`/`@firstCycle`), `battle_tank_status.c`, `sound_command_loop_count0` переписаны на явные `goto`. Расхождений с ASM не найдено; машинный код при `-O2` идентичен прежнему. Оставшиеся 5 (`sound_engine.c`: `at__`, `skip_2`, `nextSlot` — цели `BCC` вокруг `INC ptr+1`, которую C не моделирует; `equal0` в `loop_count1/2` — вход через `.BYTE $2C`) не используются законно.
- Номера строк ASM приведены к эталону (romhack `Battle City (J).asm`, 8056 строк): исправлено 89 ссылок в `PORTING.md` и `/* ASM: … */` в `src/game/`; `Save_To_VRAM` → `Save_to_VRAM`; неиспользуемый `draw_title_cursor` (ASM-метки нет) удалён. Номера в свободном тексте тоже сверены: исправлены анонимная палитра в `nmi.c` (3441 → 3442), `StaffStr_Check` (3368 → 3356) и конец таблицы строк (3158 → 3159) в `PORTING.md`.
- `Null_Status`: номер строки `XXXX`/`6331` → `6315` (сверено с upstream; 6331 — это `Rise_TankStatus_Bit`).
- Удалён `#include "strings.h"` из 7 файлов `src/game/`: такого файла в проекте никогда не было (с initial commit), подхватывался системный POSIX `<strings.h>`, ни одна его функция не использовалась.
- `-Wpedantic` «label before declaration» в `ice_move` и `string_to_screen_buffer`/`save_str_to_scr_buffer`: объявления вынесены перед метками, CI без `-Wno-error=pedantic`.
- ИИ врагов застревал у металла: `check_obj` проверяет оба передних угла; `get_random_status` — `ORA #$A0`.
- Title HI-score «00»: `ptr_to_nonzero_str_elem` учитывает `Tmp_CharIndexBase`.
- Порча `Screen_Buffer` при атрибуте `$FF` в Construction — введено экранирование `$FF`.

## Известные проблемы

- `Get_Random_A` читает `Temp,X` из псевдо-zero-page `zp_bytes`, а не из живых ZP-переменных — ГСЧ не совпадает с оригиналом побитно. **Принятое отклонение**: полную модель ZP делать не планируем.
- `zero_page_viewer()` — отладочная функция, чтение ZP заглушено нулём (в C нет реального ZP mapping).

## Следующие шаги

