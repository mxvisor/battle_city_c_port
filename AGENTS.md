# AGENTS.md — Battle City (J), порт NES ASM → C

Побайтовый перевод дизассемблера NES Battle City (1985) на C + эмуляция APU/PPU на SDL2/SDL3.
Цель — **точное** поведение оригинала, а не «похожая» игра.

В начале сессии прочитай `PROGRESS.md` и `git log --oneline -10`; в конце обнови `PROGRESS.md`.

## Команды

```bash
cmake -B build -S .                      # конфиг (SDL2); -DUSE_SDL3=ON для SDL3
cmake -B build -S . -DDEBUG_SCREENS=ON   # + src/debug.c (автопрохождение к экрану, см. DBG_* в файле)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug   # -O0 -g вместо -O2 -march=native -flto
cmake --build build -j8                  # бинарник: build/battle_city
./build/battle_city [--region ntsc|pal] [--apu-filters] [--scale N]
```

- Тестов нет. Проверка = сборка без новых предупреждений + ручной прогон нужного экрана.
- CI (`.github/workflows/ci.yml`, на каждый push): SDL2/SDL3 × Release/Debug + `DEBUG_SCREENS`, флаги `-Werror -Wno-error=unused-label -Wno-error=pedantic`, 10-секундный headless-запуск. Локальный аналог:
  `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy SDL_RENDER_DRIVER=software timeout 10 ./build/battle_city` (код 124 = не упал).
- Без SDL (например, в облачном контейнере) проверяй синтаксис так:
  `for f in src/game/*.c src/nes/*.c; do gcc -std=c11 -fsyntax-only -Wall -Wextra -Wpedantic -Wno-unused-parameter -Isrc -Isrc/game -Isrc/nes $f; done`
- CHR встраивается из `data/chr.bmp` на этапе **конфигурации** CMake — после замены BMP перезапусти `cmake -B build`.

## Карта

| Путь | Что там |
|---|---|
| `src/game/` | Порт ASM. Без SDL; к эмуляции обращается только вызовами `ppu_*`/`apu_write*` (из `draw.c`, `nmi.c`, `sound_engine.c`) |
| `src/game/zeropage.*`, `bss.*` | NES RAM: переменные с ASM-именами |
| `src/game/ppu_registers.*`, `apu_registers.*` | Теневые регистры PPU/APU с ASM-именами |
| `src/game/nmi.*` | `nmi()`, `nmi_wait()`, `vblank_wait()`, джойпады, `plat_*` API (семафоры, `plat_running`, `game_exit_buf`) |
| `src/nes/` | Эмуляция: `apu_sim` (SPSC ring buffer), `ppu_sim` (VRAM → framebuffer), `config` (NTSC/PAL), `chr_load` |
| `src/sdl_init.c`, `sdl_run.c` | Окно/аудио/ввод; render-поток с фиксированным FPS вызывает `nmi()` |
| `src/debug.c` | Отладочный «автопилот» (только при `DEBUG_SCREENS`) |
| `PORTING.md` | Чеклист всех ASM-функций → C-функций со статусом сверки; конвенции JSR-целей/меток |
| `PROGRESS.md` | Текущее состояние, известные баги, следующие шаги |

Потоки: game-поток (`nes_thread`) крутит игровую логику и спит в `nmi_wait()`; render-поток раз в кадр делает `ppu_render` → `nmi()` → будит game-поток. Подробнее — `README.md`.

## Неочевидные ловушки

- **ASM нет в репозитории.** `DOCS/` в `.gitignore`. Источник: [romhack/battle-city-disassembly](https://github.com/romhack/battle-city-disassembly), файл кладётся в `DOCS/Battle City (J).asm` (8079 строк). Все номера строк (`ASM:1234`) в коде и `PORTING.md` — по нему. Если файла нет — скажи об этом, а не восстанавливай логику по памяти.
- **`-Wunused-label` — ожидаемые предупреждения.** Неиспользуемые ASM-метки сохраняются намеренно; не удаляй их ради чистой сборки.
- `.WORD` в 6502 — little-endian: `.WORD $F207` = байты `07 F2`.
- Дизассемблер ошибается: `AND #Sound_CurrentData_Ptr` — это `AND #$C0` (байт принят за адрес), `ORA #Tank_Status` — это `ORA #$A0`. Смотри контекст.
- `$FF` в `Screen_Buffer` — терминатор записи; данные `$FF` экранируются (см. `update_screen`/`attrib_to_scr_buffer`).
- Тайловые «строки» — индексы CHR, а не ASCII (`-` = `$6B`, `.` = `$69`). Таблица — `PORTING.md` §«Строковые ASM-переменные».
- `*.py` и `BUGS.md` в `.gitignore` — вспомогательные скрипты и черновики не попадут в коммит.

## Правила портирования

### Точность
- **Не угадывай — читай ASM.** Каждая маска, сдвиг и флаг важны: `AND #$F8 / LSR / LSR` — это `>> 2`.
- Не добавляй «защитный» код, которого нет в ASM (лишние `if`, обнуления, клампы).
- Приоритет правок: логика/индексы LUT/порядок байт/ветвления → побочные эффекты и состояние ZP → косметика.

### Управление потоком
- Одна ASM-функция (от `Label:` до `; End of function Label`) = **одна** C-функция. Внутренние метки → `goto`-метки с **теми же** именами, включая `End_*` перед `RTS`. Не выноси их в хелперы.
- Отдельная C-функция для метки — только если это JSR-цель из другой функции или у неё свой `; End of function`. Классификация — `PORTING.md` §«JSR-цели ≠ функции».
- `BEQ/BNE/BCS/BCC/JMP` → `goto`. Fallthrough ≠ `return`: нет `RTS` — нет `return`.
- `PLA / PLA / JMP label` (обход возврата) → код возврата, по которому caller делает `goto label`.
- `setjmp`/`longjmp` — **только** для выхода из игры.

```c
void bonus_draw(void) {
    if (Bonus_X == 0u) goto End_Bonus_Draw;
    if (BonusPts_TimeCounter == 0u) goto Bonus_NotTaken;
    /* ... */
Bonus_NotTaken:
    if ((Frame_Counter & 8u) == 0u) goto End_Bonus_Draw;
    /* fallthrough */
Draw_Bonus:
    draw_whole_spr();
End_Bonus_Draw:
    return;
}
```

### Именование и типы
- `JSR Move_Tank` → `move_tank()`; локальные `@_`, `@__` → `at_`, `at__`. Переменные — как в ASM (`Tank_X`, `Scroll_Byte`).
- Объявление в `.h`: `/* ASM: <OriginalName> (<line>) */`.
- По умолчанию `uint8_t`; 16-бит адреса — `uint16_t`; `BMI`/`BPL` → каст к `int8_t`; carry из `ADC`/`SBC` — через `uint16_t`/`int16_t`.

### Заголовки
- Без транзитивных включений: каждый `.c` включает всё, что использует. `zeropage.h`/`bss.h` — только `<stdint.h>` и переменные.
- `src/game/` не включает SDL; новых зависимостей `game/ → nes/` не добавляй (эмуляция — отдельный слой).

## Рабочий процесс

- После сверки/правки функции обнови её строку в `PORTING.md` (`[x]`/`[~]`/`[ ]` + короткий комментарий, что исправлено).
- Коммиты — на английском, в стиле истории: заголовок `Fix <что>; <что ещё>`, в теле — список `- file: что и почему`.
- `README.md` и `README.ru.md` синхронны: меняешь один — меняй второй.
