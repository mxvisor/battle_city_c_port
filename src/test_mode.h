#ifndef TEST_MODE_H
#define TEST_MODE_H

#include <stdint.h>

/* Детерминированный headless-режим для регрессионных тестов (см. test_mode.c). */
extern int test_mode_active;

/* Запускает игру на frames кадров без SDL и потоков. Каждые every кадров
 * пишет "<кадр> <хэш кадрового буфера> <хэш записей в APU>" в out_path
 * (NULL — stdout), при dump_dir != NULL сохраняет кадр в BMP.
 * input_path — скрипт ввода "<кадр> <p1 hex> [<p2 hex>]" (NULL — без ввода). */
int test_mode_run(unsigned frames, unsigned every, const char *input_path,
                  const char *out_path, const char *dump_dir);

/* Хуки для plat_* API (sdl_init.c) в тестовом режиме. */
void test_mode_sem_wait(void);
uint8_t test_mode_buttons(int player);

#endif
