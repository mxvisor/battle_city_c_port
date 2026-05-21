#include "score.h"
#include "zeropage.h"
#include "bss.h"

/* ASM: Add_Score (4255). X*8+6 даёт линейный байтовый offset от HiScore_1P_String:
 *   X=0 → HiScore_1P_String, X=1 → HiScore_2P_String,
 *   X=2 → Temp_1PPts_String, X=3 → Temp_2PPts_String.
 * Все четыре строки в C-порту представлены отдельными массивами. */
void add_score(uint8_t x) {
    uint8_t *score_str = (x == 0u) ? HiScore_1P_String
                       : (x == 1u) ? HiScore_2P_String
                       : (x == 2u) ? Temp_1PPts_String
                       :             Temp_2PPts_String;
    uint8_t y = 6u;
    uint8_t a;
    uint8_t carry = 0u; /* CLC перед циклом */

at_:
    /* LDA Num_String,Y; ADC HiScore_1P_String,X */
    a = (uint8_t)(Num_String[y] + score_str[y] + carry);
    /* CMP #$A; BMI @__ */
    if (a < 0x0Au) goto at__;
    /* SEC; SBC #$A; SEC; JMP @___ */
    a = (uint8_t)(a - 0x0Au);
    carry = 1u;
    goto at___;

at__:
    /* CLC */
    carry = 0u;

at___:
    score_str[y] = a;
    /* DEX; DEY; BPL @_ */
    if (y == 0u) return;
    y--;
    goto at_;
}

/* ASM: Add_Life (2897). HQ destroyed → don't check; иначе для каждого игрока:
 * AddLife_Flag==0 && HiScore_XP_String+2 >= 2 → +1 жизнь, AddLife_Flag=1, играем звук. */
void add_life(uint8_t player_slot) {
    (void)player_slot;
    /* LDA HQ_Status; CMP #$80; BNE End_Add_Life */
    if (HQ_Status != 0x80u) goto End_Add_Life;
    /* LDA AddLife_Flag; BNE @_ */
    if (AddLife_Flag[0] != 0u) goto at_;
    /* LDA HiScore_1P_String+2; CMP #2; BCC @_ — взять если < 2 */
    if (HiScore_1P_String[2] < 2u) goto at_;
    Player1_Lives++;
    AddLife_Flag[0] = 1u;
    goto Play_SndAncillaryLife;

at_:
    /* LDA CursorPos; BEQ End_Add_Life */
    if (CursorPos == 0u) goto End_Add_Life;
    /* LDA AddLife_Flag+1; BNE End_Add_Life */
    if (AddLife_Flag[1] != 0u) goto End_Add_Life;
    /* LDA HiScore_2P_String+2; CMP #2; BCC End_Add_Life */
    if (HiScore_2P_String[2] < 2u) goto End_Add_Life;
    Player2_Lives++;
    AddLife_Flag[1] = 1u;

Play_SndAncillaryLife:
    Snd_Ancillary_Life1 = 1u;
    Snd_Ancillary_Life2 = 1u;

End_Add_Life:
    return;
}

