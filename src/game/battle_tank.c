#include "battle_tank.h"
#include "battle_tank_status.h"
#include "zeropage.h"
#include "bss.h"
#include "draw.h"
#include "coords.h"


/* ASM: Detect_Motion (4603). Возвращает 1 если игрок жмёт направление и tank жив. */
uint8_t detect_motion(uint8_t x) {
    uint8_t buttons = (x == 0u) ? Joypad1_Buttons : Joypad2_Buttons;
    /* LDA Joypad1_Buttons,X; AND #$F0; BEQ End_Detect_Motion */
    if ((buttons & 0xF0u) == 0u) goto End_Detect_Motion;
    /* LDA Tank_Status,X; BEQ End_Detect_Motion */
    if (Tank_Status[x] == 0u) goto End_Detect_Motion;
    return 1u;

End_Detect_Motion:
    return 0u;
}

/* ASM: Check_BorderReach (1690). Зажимает Tank_X/Y в диапазон [$18..$D8]. */
void check_border_reach(uint8_t slot) {
    /* LDA Tank_X; CMP #$D8; BCC @_; LDA #$D8; STA Tank_X */
    if (Tank_X[slot] < 0xD8u) goto at_;
    Tank_X[slot] = 0xD8u;
at_:
    /* CMP #$18; BCS @__ */
    if (Tank_X[slot] >= 0x18u) goto at__;
    Tank_X[slot] = 0x18u;
at__:
    if (Tank_Y[slot] < 0xD8u) goto at___;
    Tank_Y[slot] = 0xD8u;
at___:
    if (Tank_Y[slot] >= 0x18u) goto End_Check_BorderReach;
    Tank_Y[slot] = 0x18u;
End_Check_BorderReach:
    return;
}

void ice_detect(uint8_t slot) {
    (void)slot;

    Counter = 7;
    goto loop;

next_Tank: /* ASM: @next_Tank */
    if (Counter-- != 0u) {
        goto loop;
    }
    return;

loop: /* ASM: @loop */
    {
        uint8_t status = Tank_Status[Counter];
        if ((int8_t)status >= 0) {
            goto next_Tank;
        }

        if (status >= 0xE0u) {
            goto next_Tank;
        }

        /* ASM @5831: LDA Tank_Y,X; SEC; SBC #8; TAY; LDA Tank_X,X; SEC; SBC #8; TAX;
         * JSR GetCoord_InTiles (direct — Spr_X/Spr_Y не трогаем) */
        get_coord_in_tiles_xy(
            (uint8_t)(Tank_X[Counter] - 8u),
            (uint8_t)(Tank_Y[Counter] - 8u));
        NTAddr_Coord_Lo[Counter] = LowPtr_Byte;
        NTAddr_Coord_Hi[Counter] = HighPtr_Byte & 0x03u;

        if (Counter < 2u) {
            uint16_t addr = (uint16_t)LowPtr_Byte | ((uint16_t)HighPtr_Byte << 8);
            uint8_t tile = NT_Buffer[(addr + 0x21u) & 0x3FFu];
            if (tile == 0x21u) {
                Player_Ice_Status[Counter] |= 0x80u;
            } else {
                Player_Ice_Status[Counter] &= 0x7Fu;
            }
        }

        Temp = 0x21u;
        rise_nt_high_bit();

        if ((Tank_X[Counter] & 0x07u) == 0u) {
            NTAddr_Coord_Hi[Counter] |= 0x80u;
            Temp = 0x20u;
            rise_nt_high_bit();
        }

        if ((Tank_Y[Counter] & 0x07u) == 0u) {
            NTAddr_Coord_Hi[Counter] |= 0x40u;
            Temp = 0x01u;
            rise_nt_high_bit();
        }
    }
    goto next_Tank;
}

/* ASM: Invisible_Timer_Handle (6091). Для 2 игроков: если есть таймер силового
 * поля — раз в 64 кадра уменьшает его и рисует анимацию (2 фрейма). */
void invisible_timer_handle(uint8_t unused) {
    (void)unused;
    Counter = 1u;

loop_: /* ASM: @loop */
    /* LDA Invisible_Timer,X; BEQ @next */
    if (Invisible_Timer[Counter] == 0u) goto next_Invisible_Timer_Handle;
    /* LDA Frame_Counter; AND #63; BNE @_ */
    if ((Frame_Counter & 63u) != 0u) goto at_;
    Invisible_Timer[Counter] = (uint8_t)(Invisible_Timer[Counter] - 1u);

at_: /* ASM: @_ */
    TSA_Pal = 2u;
    Temp_X = Tank_X[Counter];
    Temp_Y = Tank_Y[Counter];
    /* LDA Frame_Counter; AND #2; ASL A; CLC; ADC #$29 */
    Spr_TileIndex = (uint8_t)(((Frame_Counter & 2u) << 1u) + 0x29u);
    draw_whole_spr();

next_Invisible_Timer_Handle: /* ASM: @next_Invisible_Timer_Handle */
    Counter = (uint8_t)(Counter - 1u);
    if ((int8_t)Counter >= 0) goto loop_;
}

void ice_move(uint8_t slot) {
    (void)slot;
    if ((Frame_Counter & 1u) != 0u) {
        goto at_; /* ASM: BNE @_ — odd frame: skip second check, go process players */
    }
    if ((Frame_Counter & 3u) != 0u) {
        goto End_Ice_Move;    /* ASM: BNE End_Ice_Move — frame%4==2: skip */
    }

at_: /* ASM: @_ */
    uint8_t player = 1;

loop: /* ASM: @loop */
    {
        uint8_t status = Tank_Status[player];
        if ((int8_t)status >= 0) {
            goto nextTank;  /* ASM: BPL @nextTank — skip if bit 7 = 0 (dead/exploding, status < 0x80) */
        }

        if (status >= 0xE0u) {
            goto nextTank;  /* skip spawning tanks */
        }

        uint8_t blink = Player_Blink_Timer[player];
        if (blink == 0u) {
            goto at____;
        }
        Player_Blink_Timer[player] = blink - 1;
        goto usual_Tank;

at____: /* ASM: @____ */
        uint8_t ice_status = Player_Ice_Status[player];
        if ((int8_t)ice_status >= 0) {
            goto at___;
        }
        if ((ice_status & 0x10u) != 0u) {
            goto usual_Tank;
        }

at___: /* ASM: @___ */
        uint8_t button_state = player == 0 ? Joypad1_Buttons : Joypad2_Buttons;
        uint8_t dir = button_to_direction_index(button_state);
        Temp = dir;
        if ((int8_t)dir < 0) {
            goto usual_Tank;
        }
        goto iceStatus;

iceStatus: /* ASM: @iceStatus */
        {
            uint8_t ice = Player_Ice_Status[player];
            if ((int8_t)ice >= 0) {
                goto at_____;
            }
            if ((ice & 0x1Fu) != 0u) {
                goto at_____;
            }

            Player_Ice_Status[player] = 0x9Cu;
            Snd_Ice = 1;
        }

at_____: /* ASM: @_____ */
        {
            uint8_t current_status = Tank_Status[player];
            uint8_t current_dir = current_status & 3u;
            if (current_dir == Temp) {
                goto skip;
            }
            if ((current_dir ^ 2u) == Temp) {
                goto skip;
            }

            uint8_t x = Tank_X[player];
            uint8_t y = Tank_Y[player];
            x = (uint8_t)((x + 4u) & 0xF8u);
            y = (uint8_t)((y + 4u) & 0xF8u);
            Tank_X[player] = x;
            Tank_Y[player] = y;
        }

skip: /* ASM: @skip */
        Tank_Status[player] = (uint8_t)(Temp | 0xA0u);
        goto nextTank;
    }

usual_Tank: /* ASM: @usual_Tank */
    Temp = 0x80u;  /* ASM: LDA #$80 before JSR Rise_TankStatus_Bit */
    rise_tank_status_bit(player);
    Tank_Status[player] |= 0x08u;
    goto nextTank;

nextTank: /* ASM: @nextTank */
    if (player-- != 0u) {
        goto loop;
    }

End_Ice_Move:
    return;
}

void motion_handle(void) {
    Counter = 7u;
    if (EnemyFreeze_Timer == 0u) {
        goto Skip_TimerOps;
    }
    if ((Frame_Counter & 0x3Fu) != 0u) {
        goto Skip_TimerOps;
    }
    EnemyFreeze_Timer--;

Skip_TimerOps:
    if (Counter < 2u) {
        if ((Frame_Counter & 1u) != 0u) {
            goto JumpToStatusHandle;
        }
        if ((Frame_Counter & 3u) != 0u) {
            goto Motion_Handle_Next;
        }
        goto JumpToStatusHandle;
    }
    goto Enemy;

Enemy:
    if (EnemyFreeze_Timer == 0u) {
        goto at_;
    }
    {
        uint8_t status = Tank_Status[Counter];
        if ((int8_t)status >= 0) {
            goto at_;
        }
        if (status < 0xE0u) {
            goto Motion_Handle_Next;
        }
    }

at_: /* ASM: @_ */
    {
        uint8_t type = Tank_Type[Counter] & 0xF0u;
        if (type == 0xA0u) {
            goto JumpToStatusHandle;
        }
        uint8_t test = Counter ^ Frame_Counter;
        if ((test & 1u) == 0u) {
            goto Motion_Handle_Next;
        }
    }

JumpToStatusHandle:
    status_core(Counter);

Motion_Handle_Next:
    Counter--;
    if ((int8_t)Counter >= 0) {
        goto Skip_TimerOps;
    }
    return;
}

/* ASM: Null_Status (6331). LDA #0; LDX #7; @_: STA Tank_Status,X; STA Player_Ice_Status,X; DEX; BPL @_ */
void null_status(void) {
    uint8_t x = 7u;
at_:
    Tank_Status[x] = 0u;
    /* ASM пишет STA Player_Ice_Status,X — в C-порту массив всего из 2 элементов;
     * для x>=2 запись игнорируется (несуществующие индексы) */
    if (x < 2u) Player_Ice_Status[x] = 0u;
    x = (uint8_t)(x - 1u);
    if ((int8_t)x >= 0) goto at_;
}

void rise_tank_status_bit(uint8_t slot) {
    Tank_Status[slot] &= 0x0F;
    Tank_Status[slot] |= Temp;
}

void rise_nt_high_bit(void) {
    /* ASM: LDA (LowPtr_Byte),Y; ORA #$80; STA (LowPtr_Byte),Y
       Temp is used as Y register offset. HighPtr_Byte already has 0x04 base
       from get_coord_in_tiles, so (ptr & 0x3FF) maps directly into NT_Buffer. */
    uint16_t addr = (uint16_t)LowPtr_Byte | ((uint16_t)HighPtr_Byte << 8);
    addr = (uint16_t)(addr + Temp);
    NT_Buffer[addr & 0x3FFu] |= 0x80u;
}

void hide_hi_bit_in_buffer(uint8_t slot) {
    /* ASM: LDA (LowPtr_Byte),Y; AND #$7F; STA (LowPtr_Byte),Y
       Temp is used as Y offset. */
    (void)slot;
    uint16_t addr = (uint16_t)LowPtr_Byte | ((uint16_t)HighPtr_Byte << 8);
    addr = (uint16_t)(addr + Temp);
    NT_Buffer[addr & 0x3FFu] &= 0x7Fu;
}

void hide_hi_bit_under_tank(uint8_t slot) {
    /* ASM: HideHiBit_Under_Tank (5895) */
    (void)slot;
    Counter = 7u;
at_: /* ASM: @_ */
    {
        uint8_t status = Tank_Status[Counter];
        if ((int8_t)status >= 0) {
            goto at___;
        }
        if (status >= 0xE0u) {
            goto at___;
        }

        LowPtr_Byte = NTAddr_Coord_Lo[Counter];
        HighPtr_Byte = (uint8_t)((NTAddr_Coord_Hi[Counter] & 0x03u) | 0x04u);

        Temp = 0x21u;
        hide_hi_bit_in_buffer(0);

        if ((NTAddr_Coord_Hi[Counter] & 0x80u) == 0u) {
            goto at__;
        }
        Temp = 0x20u;
        hide_hi_bit_in_buffer(0);

at__: /* ASM: @__ */
        if ((NTAddr_Coord_Hi[Counter] & 0x40u) == 0u) {
            goto at___;
        }
        Temp = 0x01u;
        hide_hi_bit_in_buffer(0);
    }

at___: /* ASM: @___ */
    Counter--;
    if ((int8_t)Counter >= 0) {
        goto at_;
    }
    return;
}
uint8_t button_to_direction_index(uint8_t buttons) {
    /* ASM: Button_To_DirectionIndex (6373) */
    uint8_t a = buttons;
    uint8_t carry = (uint8_t)(a & 0x80u);
    a <<= 1; /* ASL A */
    if (carry == 0u) {
        goto at_;
    }
    return 3u; /* Right */

at_: /* ASM: @_ */
    carry = (uint8_t)(a & 0x80u);
    a <<= 1; /* ASL A */
    if (carry == 0u) {
        goto at__;
    }
    return 1u; /* Left */

at__: /* ASM: @__ */
    carry = (uint8_t)(a & 0x80u);
    a <<= 1; /* ASL A */
    if (carry == 0u) {
        goto at___;
    }
    return 2u; /* Down */

at___: /* ASM: @___ */
    carry = (uint8_t)(a & 0x80u);
    a <<= 1; /* ASL A */
    if (carry == 0u) {
        goto at____;
    }
    return 0u; /* Up */

at____: /* ASM: @____ */
    return 0xFFu; /* Direction keys not pressed */
}
