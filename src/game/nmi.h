#ifndef NMI_H
#define NMI_H

#include <stdint.h>
#include <stdatomic.h>
#include <setjmp.h>

typedef void* plat_sem_t;

extern volatile int plat_running;

void plat_sem_wait(plat_sem_t sem);
void plat_sem_post(plat_sem_t sem);

uint8_t plat_poll_buttons_p1(void);
uint8_t plat_poll_buttons_p2(void);

plat_sem_t plat_get_wake_sem(void);
plat_sem_t plat_get_vblank_sem(void);

extern atomic_int game_in_nmi_wait;
extern jmp_buf game_exit_buf;

void nmi(void);
void nmi_wait(void);
void vblank_wait(void);
void read_joypads(void);
void update_screen(void);
void spr_pal_load(void);
void load_bkg_pal(void);
/* ASM: Spr_Pal_Load (3419) */
void spr_pal_load(void);
/* ASM: Spr_Invisible (4446) */
void spr_invisible(void);

#endif // NMI_H
