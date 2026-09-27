# PROGRESS.md — состояние проекта

Читать в начале сессии вместе с `git log --oneline -10`. Обновлять в конце сессии:
что сделано, что сломано, что дальше. Коротко; история — в git, детали сверки — в `PORTING.md`.

_Обновлено: 2026-09-27_

## Текущее состояние

- Все **218/218** top-level ASM-функций портированы и отмечены `[x]` в `PORTING.md`.
- Работает: title, construction, stage select, бой (1P/2P), demo, pts-screen, game over, hi-score, secret message; NTSC/PAL; SDL2 и SDL3.
- Тестов нет. CI (GitHub Actions) — сборка SDL2/SDL3 × Release/Debug с `-Werror` (кроме `unused-label`) + headless smoke-запуск на 10 с.

## Последние исправления (см. `git log`)

- `Null_Status`: номер строки `XXXX`/`6331` → `6315` (сверено с upstream; 6331 — это `Rise_TankStatus_Bit`).
- Удалён `#include "strings.h"` из 7 файлов `src/game/`: такого файла в проекте никогда не было (с initial commit), подхватывался системный POSIX `<strings.h>`, ни одна его функция не использовалась.
- `-Wpedantic` «label before declaration» в `ice_move` и `string_to_screen_buffer`/`save_str_to_scr_buffer`: объявления вынесены перед метками, CI без `-Wno-error=pedantic`.
- ИИ врагов застревал у металла: `check_obj` проверяет оба передних угла; `get_random_status` — `ORA #$A0`.
- Title HI-score «00»: `ptr_to_nonzero_str_elem` учитывает `Tmp_CharIndexBase`.
- Порча `Screen_Buffer` при атрибуте `$FF` в Construction — введено экранирование `$FF`.

## Известные проблемы

- Номера строк ASM: 71 из ~290 ссылок (`PORTING.md` + `/* ASM: … */` в `src/game/*.h`) не совпадают с `^Label:` в upstream `Battle City (J).asm` (romhack, 8056 строк; в `AGENTS.md` указано 8079 — возможно, локальная копия отличается). Примеры: `Button_To_DirectionIndex` 6644 → 6373, `Hide_All_Bullets` 6315 → 6301.
- `zero_page_viewer()` — отладочная функция, чтение ZP заглушено нулём (в C нет реального ZP mapping).

## Следующие шаги

- [ ] Сверить и исправить 71 расходящийся номер строки ASM (после проверки, какая версия `.asm` — эталон).
- [ ] Повторная выборочная сверка функций, где были недавние баги (коллизии, AI, звук), с ASM.
