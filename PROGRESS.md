# PROGRESS.md — состояние проекта

Читать в начале сессии вместе с `git log --oneline -10`. Обновлять в конце сессии:
что сделано, что сломано, что дальше. Коротко; история — в git, детали сверки — в `PORTING.md`.

_Обновлено: 2026-09-27_

## Текущее состояние

- Все **218/218** top-level ASM-функций портированы и отмечены `[x]` в `PORTING.md`.
- Работает: title, construction, stage select, бой (1P/2P), demo, pts-screen, game over, hi-score, secret message; NTSC/PAL; SDL2 и SDL3.
- Тестов нет. CI (GitHub Actions) — сборка SDL2/SDL3 × Release/Debug с `-Werror` (кроме `unused-label`) + headless smoke-запуск на 10 с.

## Последние исправления (см. `git log`)

- Неиспользуемые ASM-метки (18 → 5): ветвления `BPL`/`BEQ`/`BNE` в `battle_collide.c`, `battle_respawn.c` (`load_new_tank`, + пропущенные `@enemiesLeft`/`@firstCycle`), `battle_tank_status.c`, `sound_command_loop_count0` переписаны на явные `goto`. Расхождений с ASM не найдено; машинный код при `-O2` идентичен прежнему. Оставшиеся 5 (`sound_engine.c`: `at__`, `skip_2`, `nextSlot` — цели `BCC` вокруг `INC ptr+1`, которую C не моделирует; `equal0` в `loop_count1/2` — вход через `.BYTE $2C`) не используются законно.
- Номера строк ASM приведены к эталону (romhack `Battle City (J).asm`, 8056 строк): исправлено 89 ссылок в `PORTING.md` и `/* ASM: … */` в `src/game/`; `Save_To_VRAM` → `Save_to_VRAM`; `draw_title_cursor` помечен как C-port helper (ASM-метки нет). Диапазоны `(ASM:x–y)` в списке goto-якорей были верны. Номера внутренних меток в свободном тексте (`line 3014` и т.п.) не проверялись.
- `Null_Status`: номер строки `XXXX`/`6331` → `6315` (сверено с upstream; 6331 — это `Rise_TankStatus_Bit`).
- Удалён `#include "strings.h"` из 7 файлов `src/game/`: такого файла в проекте никогда не было (с initial commit), подхватывался системный POSIX `<strings.h>`, ни одна его функция не использовалась.
- `-Wpedantic` «label before declaration» в `ice_move` и `string_to_screen_buffer`/`save_str_to_scr_buffer`: объявления вынесены перед метками, CI без `-Wno-error=pedantic`.
- ИИ врагов застревал у металла: `check_obj` проверяет оба передних угла; `get_random_status` — `ORA #$A0`.
- Title HI-score «00»: `ptr_to_nonzero_str_elem` учитывает `Tmp_CharIndexBase`.
- Порча `Screen_Buffer` при атрибуте `$FF` в Construction — введено экранирование `$FF`.

## Известные проблемы

- `zero_page_viewer()` — отладочная функция, чтение ZP заглушено нулём (в C нет реального ZP mapping).

## Следующие шаги

- [ ] Повторная выборочная сверка функций, где были недавние баги (коллизии, AI, звук), с ASM.
