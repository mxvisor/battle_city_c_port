#include "score.h"
#include "zeropage.h"
#include "bss.h"

void add_score(uint8_t player) {
    /* ASM: Add_Score (4255)
     * X*8+6 = offset from HiScore_1P_String in zero-page:
     *   0 → HiScore_1P_String, 1 → HiScore_2P_String,
     *   2 → Temp_1PPts_String, 3 → Temp_2PPts_String */
    uint8_t *score_str = (player == 0u) ? HiScore_1P_String
                       : (player == 1u) ? HiScore_2P_String
                       : (player == 2u) ? Temp_1PPts_String
                       :                  Temp_2PPts_String;
    uint8_t carry = 0u;
    for (int8_t y = 6; y >= 0; y--) {
        uint16_t sum = (uint16_t)score_str[(uint8_t)y]
                     + (uint16_t)Num_String[(uint8_t)y]
                     + (uint16_t)carry;
        if (sum >= 10u) {
            carry = 1u;
            sum = (uint16_t)(sum - 10u);
        } else {
            carry = 0u;
        }
        score_str[(uint8_t)y] = (uint8_t)sum;
    }
}

void add_life(uint8_t player_slot) {
    /* ASM: Add_Life (2897)
     * Gives an extra life when either player's score reaches 20000. */
    (void)player_slot;
    if (HQ_Status != 0x80u) return;
    if (AddLife_Flag[0] == 0u) {
        if (HiScore_1P_String[2] >= 2u) {
            Player1_Lives++;
            AddLife_Flag[0] = 1u;
            goto Play_SndAncillaryLife;
        }
    }
    if (CursorPos == 0u) return;
    if (AddLife_Flag[1] != 0u) return;
    if (HiScore_2P_String[2] < 2u) return;
    Player2_Lives++;
    AddLife_Flag[1] = 1u;
Play_SndAncillaryLife:
    Snd_Ancillary_Life1 = 1u;
    Snd_Ancillary_Life2 = 1u;
}

