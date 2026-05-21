#ifndef DEMO_LEVEL_H
#define DEMO_LEVEL_H

#include <stdint.h>

/* ASM: Load_DemoLevel (820) */
void load_demo_level(void);

/* ASM: BonusLevel_ButtonCheck (873).
 * Возвращает: 0 — End_Demo (нормальный выход → New_Scroll в caller);
 *             1 — Button_Pressed (ASM `JMP Title_Loaded` → `goto title_loaded` в caller). */
int bonus_level_button_check(void);

/* ASM: Demo_AI — простая AI для demo-уровня. */
void demo_ai(void);

#endif // DEMO_LEVEL_H
