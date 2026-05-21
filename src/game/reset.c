#include "reset.h"
#include "zeropage.h"
#include "bss.h"
#include "draw.h"
#include "ppu_registers.h"
#include "nmi.h"
#include "strings.h"
#include "sound_engine.h"
#include <string.h>
#include <stdint.h>

static const uint8_t StaffString[48] = "RYOUITI OOKUBO  TAKEFUMI HYOUDOU" "JUNKO OZAWA     ";

static void load_pals(void) {
    vblank_wait();
    spr_pal_load();
    BkgPal_Number = 0;
    load_bkg_pal();
}

void reset_screen_stuff(void) {
    memset(Screen_Buffer, 0, sizeof(Screen_Buffer));

    Char_Index_Base = 0;
    Tmp_CharIndexBase = 0;
    ScrBuffer_Pos = 0;
    SprBuffer_Position = 0;
    Pause_Flag = 0;
    BkgPal_Number = 0xFF;

    load_pals();

    Gap = 4;
    Spr_Attrib = 0x20;

    null_nt_buffer();
    spr_invisible();

    null_8bytes_string(HiScore_1P_String);
    null_8bytes_string(HiScore_2P_String);

    if (staff_str_check() != 0) {
        goto hot_boot;
    }

    null_8bytes_string(HiScore_String);
    HiScore_String[2] = 2;
    CursorPos = 0;

hot_boot:
    PPU_Addr_Ptr = 0x1C;
    store_nt_buffer_in_vram();

    PPU_Addr_Ptr = 0x24;
    store_nt_buffer_in_vram();

    staff_str_store();
    sound_stop();

    PPU_REG1_Stts = 0;
    Scroll_Byte = 0;
}

void reset(void) {
    PPU_CTRL_REG1 = 0x10;
    // PPU status wait loop skipping in C port
    PPU_CTRL_REG2 = 0x06;
    reset_screen_stuff();
    Scroll_Byte = 0;
    PPU_REG1_Stts = 0;
    set_ppu();
}

/* ASM: StaffStr_Store — LDX #$F; @_: LDA StaffString,X; STA StaffString_RAM,X; DEX; BPL @_ */
void staff_str_store(void) {
    uint8_t x = 0x0Fu;
at_:
    StaffString_RAM[x] = StaffString[x];
    x = (uint8_t)(x - 1u);
    if ((int8_t)x >= 0) goto at_;
}

/* ASM: StaffStr_Check — LDX #$F; @_: ... BNE ColdBoot; DEX; BPL @_; LDA #1; RTS; ColdBoot: LDA #0; RTS */
uint8_t staff_str_check(void) {
    uint8_t x = 0x0Fu;
at_:
    if (StaffString_RAM[x] != StaffString[x]) goto ColdBoot;
    x = (uint8_t)(x - 1u);
    if ((int8_t)x >= 0) goto at_;
    return 1u;

ColdBoot:
    return 0u;
}

