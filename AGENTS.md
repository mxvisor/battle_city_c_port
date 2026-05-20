# AGENTS.md — Правила портирования Battle City (J)

## Оригинальный ASM

Основной дизассемблер:
- `DOCS/Battle City (J).asm` — полный дизассемблер (8079 строк)

При портировании всегда сверяйся с `Battle City (J).asm` по номерам строк и меткам.

## Чеклист соответствия

- `PORTING.md` — список всех top-level функций из `Battle City (J).asm` с маппингом на C-функции и пометками статуса проверки (`[x]` сверено, `[~]` без сверки, `[ ]` не проверено). Там же конвенция о ASM-метках, которые в C превращаются в `goto`-якоря или сворачиваются внутрь других функций. Обновляй чеклист при каждой проверке/правке функции.

## Правила портирования

### 1. Точное соответствие ASM

- **Не угадывай** — читай ASM. Каждая инструкция, каждая маска, каждый сдвиг имеют значение.
- Если в ASM `AND #$F8 / LSR / LSR` — это `>> 2`, а не `>> 1`. Проверяй битовые операции побайтово.
- `.WORD` в 6502 — **little-endian** (low byte first). `.WORD $F207` = байты `07 F2`. При чтении как `uint16_t` в C значение будет `0x07F2`, а не `0xF207`.
- Дизассемблер может ошибаться: `AND #Sound_CurrentData_Ptr` может быть артефактом, где байт `$C0` интерпретирован как адрес. Смотри контекст и комментарии.

### 2. Управление потоком

- **Используй `goto`** для прямого перевода ветвлений ASM (`BEQ`, `BNE`, `BCS`, `BCC`, `JMP`).
- Fallthrough — это не `return`. Если ASM не делает `RTS`, C не делает `return`.
- **Fallthrough-функции:** часто встречается, что после `; End of function ...` нет `RTS`. Код просто переходит в следующую функцию/метку. В C это `goto` на следующую метку или объединение логики.
- `PLA / PLA / JMP label` — это обход возврата к вызывающему. В C это `goto label` или `return` с последующим переходом в caller.
- `setjmp`/`longjmp` — **только** для выхода из игры. Обычный поток — `return`/`goto`.

#### 2.1. Сохраняй структуру меток внутри функции

ASM-функция между `Label:` и `; End of function Label` — это **одна** C-функция. Все её внутренние метки (`NotZeroCounter:`, `Bonus_NotTaken:`, `Draw_Bonus:`, `End_Bonus_Draw:` и т.п.) переводятся в `goto`-метки **внутри той же** C-функции, а не в отдельные хелперы.

❌ **Неправильно** — распилить на helper:
```c
void bonus_draw(void) {
    if (Bonus_X == 0u) return;
    /* ... */
    draw_bonus();  // вынесен в отдельную функцию
}
void draw_bonus(void) { /* Draw_Bonus label body */ }
```

✓ **Правильно** — одна функция с `goto`:
```c
void bonus_draw(void) {
    if (Bonus_X == 0u) goto End_Bonus_Draw;
    if (BonusPts_TimeCounter == 0u) goto Bonus_NotTaken;
    BonusPts_TimeCounter--;
    if (BonusPts_TimeCounter != 0u) goto NotZeroCounter;
    Bonus_X = 0u;
    goto End_Bonus_Draw;

NotZeroCounter:
    TSA_Pal = 2u;
    Spr_TileIndex = 0x3Bu;
    goto Draw_Bonus;

Bonus_NotTaken:
    if ((Frame_Counter & 8u) == 0u) goto End_Bonus_Draw;
    TSA_Pal = 2u;
    Spr_TileIndex = (uint8_t)((Bonus_Number << 2) + 0x81u);
    /* fallthrough */
Draw_Bonus:
    Temp_X = Bonus_X;
    Temp_Y = Bonus_Y;
    Spr_Attrib = 0u;
    draw_whole_spr();
    Spr_Attrib = 0x20u;
End_Bonus_Draw:
    return;
}
```

Имена меток в C — **те же**, что в ASM (включая `End_*`-метку перед `RTS`). Это сохраняет читаемость 1-в-1 с дизассемблером и упрощает сверку.

**Исключения**, когда метка становится отдельной C-функцией:
- Метка — JSR-цель из **другой** функции (см. §2 раздел «Замечание: JSR-цели ≠ функции» в `PORTING.md`).
- Метка — самостоятельная функция в дизассемблере (есть свой `; End of function`).

### 3. Заголовочные файлы

- **Нет транзитивных включений.** `zeropage.h` содержит только `<stdint.h>` и объявления zeropage-переменных.
- Каждый `.c` файл включает **все** заголовки, которые ему нужны напрямую.
- `render.h` **запрещено** включать в игровые модули (`game/*.c`). Рендер — отдельный слой.

### 4. Именование

- Функции: ASM `JSR CamelCase` → C `snake_case` (например, `JSR Move_Tank` → `move_tank()`)
- Локальные метки ASM: `@_` → `at_`, `@__` → `at__`, `@___` → `at___` и т.д.
- Переменные: те же имена, что в ASM (`Scroll_Byte`, `BkgPal_Number`, `Tank_X`)
- В `.h` файлах: `/* ASM <OriginalName> (line <N>) */` в объявлении функции

### 5. Типы данных

- Всё `uint8_t` по умолчанию, если ASM работает с 8-бит значениями.
- 16-бит адресация — `uint16_t`.
- Знаковые сравнения (`BMI`/`BPL`) — каст к `int8_t`.
- Carry из `ADC`/`SBC` — раскладка через `uint16_t`/`int16_t` промежуточное.

### 6. Сборка

- **Система сборки:** CMake
- **Конфигурация:** `cmake -B build -S .`
- **Компиляция:** `cmake --build build` или `make -C build -j8`
- **Бинарник:** `build/battle_city`
- **Зависимости:** SDL2 (библиотека + заголовки)
- **CHR-данные:** встроены в бинарник на этапе сборки через `file(READ ... HEX)` в `CMakeLists.txt`
- **Флаги оптимизации:** `-O2 -march=native -flto -Wall -Wextra -Wpedantic`

### 10. Приоритеты исправлений

1. **CRITICAL** — логика расчётов, индексы LUT, порядок байт, ветвления
2. **MEDIUM** — побочные эффекты, указатели, состояние zero-page
3. **LOW** — лишние записи регистров, косметические расхождения
